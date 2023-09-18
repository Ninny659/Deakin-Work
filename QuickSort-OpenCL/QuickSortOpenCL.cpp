#include <iostream>
#include <random>
#include <utility>
#include <functional>
#include <chrono>
#include <iostream>
#include <mpi.h>
#include <CL/cl.h>

using namespace std;

int N = 10000;
// Initialise localChunkSize
int localChunkSize = N / 2;

int *unsortedArray;
// Set up localChunk and sortedChunk
int *localChunk = (int *)malloc(localChunkSize * sizeof(int));
int *sortedChunk = (int *)malloc(localChunkSize * sizeof(int));

// OpenCL variables
cl_mem bufUnsorted;
cl_mem bufSorted;

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

// Function to check if an array is sorted
void checkSortedArray(int *arr)
{
    // Check if the array is sorted
    for (int i = 0; i < N - 1; i++)
    {
        if (arr[i] > arr[i + 1])
        {
            cout << "Array is not sorted" << endl;
            return;
        }
    }
    cout << "Array is sorted" << endl;
}

// Function to generate an array with random values
int *unsortedArrayGenerator()
{
    int *unsortedArray = (int *)malloc(N * sizeof(int));

    srand(time(NULL));
    for (int i = 0; i < N; i++)
    {
        unsortedArray[i] = (rand() % 100) + 1;

        // For testing purposes, we can use the following line to generate an array with values from 1 to N
        // unsortedArray[i] = i;
    }

    return unsortedArray;
}

// Timer function
double timerFunction(std::function<void(void)> timedFunc)
{
    // Starting the timer and then running the function. Keeps it very clean.
    auto start = std::chrono::high_resolution_clock::now();

    timedFunc();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    return elapsed.count();
}

// Function to merge two sorted arrays
int* merge(int* sortedChunk1, int sizeChunk1, int* sortedChunk2, int sizeChunk2)
{
    int* mergedArray = (int*)malloc((sizeChunk1 + sizeChunk2) * sizeof(int));
    int i, j, k = 0;


    // Traverse both arrays simultaneously and store the smallest element of both in the mergedArray array
    while (i < sizeChunk1 && j < sizeChunk2) {
        if (sortedChunk1[i] < sortedChunk2[j]) mergedArray[k++] = sortedChunk1[i++];
        else mergedArray[k++] = sortedChunk2[j++];
    }

    // Store the remaining elements of the first array
    while (i < sizeChunk1) mergedArray[k++] = sortedChunk1[i++];
    while (j < sizeChunk2) mergedArray[k++] = sortedChunk2[j++];

    return mergedArray;
}

// Function to sort an array using quick sort
void fastort(int *unsortedArray, int argc, char *argv[])
{
    // Initialize MPI (assuming you have MPI configured and initialized elsewhere)
    MPI_Init(&argc, &argv);
    int numProcs, rank = 0;
    MPI_Status status;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

    // Calculate the size of each chunk
    int localSize = N / numProcs;
    int localChunkSize;

    // If the number of elements is not divisible by the number of processes, the last process will have a smaller chunk.
    if (N >= localSize * (rank + 1)) localChunkSize = localSize;
    else localChunkSize = N - localSize * rank;

    // Initialise localChunk
    localChunk = (int *)malloc(localChunkSize * sizeof(int));

    // Scatter the array into chunks, and sort each chunk. Size is localSize.
    MPI_Scatter(unsortedArray, localSize, MPI_INT, localChunk, localSize, MPI_INT, 0, MPI_COMM_WORLD);

    setup_openCL_device_context_queue_kernel((char *)"./quick_sort.cl", (char *)"QuickSort");

    setup_kernel_memory();
    copy_kernel_args();

    // Sort the local chunk using OpenCL
    size_t global[] = {(size_t)localChunkSize};
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global, NULL, 0, NULL, &event);
    clWaitForEvents(1, &event);

    clEnqueueReadBuffer(queue, bufSorted, CL_TRUE, 0, localChunkSize * sizeof(int), localChunk, 0, NULL, NULL);

    // Merge the sorted chunks
    for (int i = 1; i < numProcs; i *= 2)
    {
        if (rank % (2 * i) != 0)
        {
            // If the process is not the first process in the pair, send the localChunk to the process with rank - i.
            MPI_Send(localChunk, localChunkSize, MPI_INT, rank - i, 0, MPI_COMM_WORLD);
            break;
        }

        // If the process is the first process in the pair, receive the localChunk from the process with rank + i.
        if (rank + i < numProcs)
        {
            int receivedChunkSize;
            if (N >= localSize * (rank + 2 * i))
                receivedChunkSize = localSize * i;
            else
                receivedChunkSize = N - localSize * (rank + i);

            int *receivedChunk = (int *)malloc(receivedChunkSize * sizeof(int));

            MPI_Recv(receivedChunk, receivedChunkSize, MPI_INT, rank + i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            int *mergedChunk = merge(localChunk, localChunkSize, receivedChunk, receivedChunkSize);
            free(localChunk);
            localChunk = mergedChunk;
            localChunkSize += receivedChunkSize;
        }
    }

    MPI_Finalize();

    // Check if the array is sorted (perform this on rank 0)
    if (rank == 0)
    {
        checkSortedArray(localChunk);

        // Debug: Print the sorted array
        // std::cout << "\nSorted Array: " << std::endl;
        // for(int i = 0; i < N; i++)
        // {
        //     printf("%d ", localChunk[i]);
        // }
    }

    free(localChunk);
}

// Helper function to run fastort
double fastortHelper()
{
    double executionTime_fastification = timerFunction([&]()
                                                       { fastort(unsortedArray, 0, NULL); });

    // Removing array from memory
    free(unsortedArray);

    return executionTime_fastification;
}

// Main function
int main()
{
    unsortedArray = unsortedArrayGenerator();

    double executionTime_fastification = 0;
    executionTime_fastification = fastortHelper();

    std::cout << "\nExecution Time for " << N << " elements (Fastort): " << executionTime_fastification << " microseconds \n" << std::endl;
    return 0;
}

void copy_kernel_args()
{
    // Copy the arguments to the kernel
    clSetKernelArg(kernel, 0, sizeof(int), (void *)&localChunkSize);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&bufSorted);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&bufUnsorted);

    if (err < 0)
    {
        perror("Couldn't create a kernel argument");
        printf("error = %d", err);
        exit(1);
    }
}

void setup_kernel_memory()
{
    bufSorted = clCreateBuffer(context, CL_MEM_READ_WRITE, localChunkSize * sizeof(int), NULL, NULL);
    bufUnsorted = clCreateBuffer(context, CL_MEM_READ_WRITE, localChunkSize * sizeof(int), NULL, NULL);

    clEnqueueWriteBuffer(queue, bufSorted, CL_TRUE, 0, localChunkSize * sizeof(int), localChunk, 0, NULL, NULL);
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

    /* Parameters for clCreateProgramWithSource:
       - context: The context associated with the program being created.
       - count: Number of strings in the array of program source strings.
       - strings: Array of strings containing the program source code.
       - lengths: Array of sizes for each string in the program source.
       - errcode_ret: Pointer to store the error code.
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