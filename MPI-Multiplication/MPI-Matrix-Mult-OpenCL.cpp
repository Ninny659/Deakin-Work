#include <iostream>
#include <random>
#include <utility>
#include <functional>
#include <chrono>
#include <fstream>
#include <CL/cl.h>
#include <mpi.h>

using namespace std;

#define PRINT 1
#define N 835
int SZ = N;

// MPI Status
MPI_Status status;

int *m1, *m2, *m3;

// buffer for the vectors
cl_mem bufM1, bufM2, bufM3;

cl_device_id device_id;
cl_context context;
cl_program program;
cl_kernel kernel;
cl_command_queue queue;
cl_event event = NULL;

int err;

cl_device_id create_device();
void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname);
cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename);
void setup_kernel_memory();
void copy_kernel_args();
void free_memory();

void init(int *&A, int size);
void print(int *A, int size);

double timerFunction(std::function<void(void)> timedFunc)
{
    auto start = std::chrono::high_resolution_clock::now();
    timedFunc();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    return elapsed.count();
}

void mpiMatrixMultiply(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    if (argc > 1) SZ = atoi(argv[1]);

    init(m1, SZ * SZ);
    init(m2, SZ * SZ);
    m3 = (int *)malloc(sizeof(int) * SZ * SZ);

    size_t global[] = {(size_t)SZ, (size_t)SZ};

    // initial vector
    print(m1, SZ * SZ);
    print(m2, SZ * SZ);

    setup_openCL_device_context_queue_kernel((char *)"./matrix_multiply.cl", (char *)"matrix_multiply");

    setup_kernel_memory();
    copy_kernel_args();

    int rank, numProc, workerTasks, taskAssignee, matrixRows, rowStartingPos = 0;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numProc);

    workerTasks = numProc - 1;

    if (rank == 0)
    {
        matrixRows = SZ / workerTasks;
        rowStartingPos = 0;

        MPI_Bcast(m2, SZ * SZ, MPI_INT, 0, MPI_COMM_WORLD);

        for (taskAssignee = 1; taskAssignee <= workerTasks; taskAssignee++)
        {
            MPI_Send(&rowStartingPos, 1, MPI_INT, taskAssignee, 1, MPI_COMM_WORLD);
            MPI_Send(&matrixRows, 1, MPI_INT, taskAssignee, 1, MPI_COMM_WORLD);
            MPI_Send(&m1[rowStartingPos * SZ], matrixRows * SZ, MPI_INT, taskAssignee, 1, MPI_COMM_WORLD);
            rowStartingPos = rowStartingPos + matrixRows;
        }

        for (int i = 1; i <= workerTasks; i++)
        {
            MPI_Recv(&rowStartingPos, 1, MPI_INT, i, 2, MPI_COMM_WORLD, &status);
            MPI_Recv(&matrixRows, 1, MPI_INT, i, 2, MPI_COMM_WORLD, &status);
            MPI_Recv(&m3[rowStartingPos * SZ], matrixRows * SZ, MPI_INT, i, 2, MPI_COMM_WORLD, &status);
        }
    }

    if (rank > 0)
    {
        MPI_Bcast(m2, SZ * SZ, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Recv(&rowStartingPos, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
        MPI_Recv(&matrixRows, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
        MPI_Recv(&m1[0], matrixRows * SZ, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);

        clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, NULL, 0, NULL, &event);
        clWaitForEvents(1, &event);

        MPI_Send(&rowStartingPos, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
        MPI_Send(&matrixRows, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
        MPI_Send(&m3[rowStartingPos * SZ], matrixRows * SZ, MPI_INT, 0, 2, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    clEnqueueReadBuffer(queue, bufM3, CL_TRUE, 0, SZ * SZ * sizeof(int), &m3[0], 0, NULL, NULL);
    print(m3, SZ * SZ);
}

double MPIOpenCLHelper()
{
    double executionTime = timerFunction([&]() {
        mpiMatrixMultiply(0, NULL);
        free_memory();
    });

    return executionTime;
}

int main()
{
    int mpiTimed = MPIOpenCLHelper();
    std::cout << "Execution Time for " << N << " elements (MPI and OpenCL): " << mpiTimed << " milliseconds" << std::endl;

    return 0;
}

void init(int *&A, int size)
{
    A = (int *)malloc(sizeof(int) * size);

    for (long i = 0; i < size; i++)
    {
        A[i] = rand() % 100; // any number less than 100
    }
}

void print(int *A, int size)
{
    if (PRINT == 0)
    {
        return;
    }

    if (PRINT == 1 && size > 15)
    {
        for (long i = 0; i < 5; i++)
        {                        // rows
            printf("%d ", A[i]); // print the cell value
        }
        printf(" ..... ");
        for (long i = size - 5; i < size; i++)
        {                        // rows
            printf("%d ", A[i]); // print the cell value
        }
    }
    else
    {
        for (long i = 0; i < size; i++)
        {                        // rows
            printf("%d ", A[i]); // print the cell value
        }
    }
    printf("\n----------------------------\n");
}

void free_memory()
{
    // free the buffers
    clReleaseMemObject(bufM1);
    clReleaseMemObject(bufM2);
    clReleaseMemObject(bufM3);

    // free opencl objects
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseProgram(program);
    clReleaseContext(context);

    free(m1);
    free(m2);
    free(m3);
}

void copy_kernel_args()
{
    clSetKernelArg(kernel, 0, sizeof(int), (void *)&SZ);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&bufM1);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&bufM2);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&bufM3);

    if (err < 0)
    {
        perror("Couldn't create a kernel argument");
        printf("error = %d", err);
        exit(1);
    }
}

void setup_kernel_memory()
{
    bufM1 = clCreateBuffer(context, CL_MEM_READ_WRITE, N * N * sizeof(int), NULL, NULL);
    bufM2 = clCreateBuffer(context, CL_MEM_READ_WRITE, N * N * sizeof(int), NULL, NULL);
    bufM3 = clCreateBuffer(context, CL_MEM_READ_WRITE, N * N * sizeof(int), NULL, NULL);

    clEnqueueWriteBuffer(queue, bufM1, CL_TRUE, 0, N * N * sizeof(int), &m1[0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, bufM2, CL_TRUE, 0, N * N * sizeof(int), &m2[0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, bufM3, CL_TRUE, 0, N * N * sizeof(int), &m3[0], 0, NULL, NULL);
}

void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname)
{
    device_id = create_device();
    cl_int err;

    // The purpose of clCreateContext is to create an OpenCL context, which is the environment within which the kernels execute.
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    if (err < 0)
    {
        perror("Couldn't create a context");
        exit(1);
    }

    program = build_program(context, device_id, filename);

    // The purpose of clCreateCommandQueueWithProperties is depreciated use clCreateCommandQueue is to create a command-queue on a specific device.
    queue = clCreateCommandQueueWithProperties(context, device_id, 0, &err);
    if (err < 0)
    {
        perror("Couldn't create a command queue");
        exit(1);
    };

    kernel = clCreateKernel(program, kernelname, &err);
    if (err < 0)
    {
        perror("Couldn't create a kernel");
        printf("error =%d", err);
        exit(1);
    };
}

cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename)
{

    cl_program program;
    FILE *program_handle;
    char *program_buffer, *program_log;
    size_t program_size, log_size;

    /* Read program file and place content into buffer */
    program_handle = fopen(filename, "r");
    if (program_handle == NULL)
    {
        perror("Couldn't find the program file");
        exit(1);
    }
    fseek(program_handle, 0, SEEK_END);
    program_size = ftell(program_handle);
    rewind(program_handle);
    program_buffer = (char *)malloc(program_size + 1);
    program_buffer[program_size] = '\0';
    fread(program_buffer, sizeof(char), program_size, program_handle);
    fclose(program_handle);

    /*
    clCreateProgramWithSource is used to compile and create an OpenCL program from provided source code strings.
    Once created, you can further build and manipulate the program, including creating kernel objects from it.
    */

    program = clCreateProgramWithSource(ctx, 1,
                                        (const char **)&program_buffer, &program_size, &err);
    if (err < 0)
    {
        perror("Couldn't create the program");
        exit(1);
    }
    free(program_buffer);

    /* Build program

   The fourth parameter accepts options that configure the compilation.
   These are similar to the flags used by gcc. For example, you can
   define a macro with the option -DMACRO=VALUE and turn off optimization
   with -cl-opt-disable.
   */
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err < 0)
    {

        /* Find size of log and print to std output */
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
                              0, NULL, &log_size);
        program_log = (char *)malloc(log_size + 1);
        program_log[log_size] = '\0';
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
                              log_size + 1, program_log, NULL);
        printf("%s\n", program_log);
        free(program_log);
        exit(1);
    }

    return program;
}

cl_device_id create_device()
{

    cl_platform_id platform;
    cl_device_id dev;
    int err;

    /* Identify a platform */
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err < 0)
    {
        perror("Couldn't identify a platform");
        exit(1);
    }

    // Access a device
    // GPU
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
    if (err == CL_DEVICE_NOT_FOUND)
    {
        // CPU
        printf("GPU not found\n");
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
    }
    if (err < 0)
    {
        perror("Couldn't access any devices");
        exit(1);
    }

    return dev;
}