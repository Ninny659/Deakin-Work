#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <CL/cl.h>

#define PRINT 1

int SZ = 8;
// int *v;
int *v1, *v2, *v3;

using namespace std;

// buffer for the vectors
// cl_mem bufV;
cl_mem bufV1, bufV2, bufV3;

// Device id for an attached GPU device
cl_device_id device_id;
// Context holds information and allows communication between host and device
cl_context context;
// Program object created from a source file
cl_program program;
// Initialises the kernel for the code to be executed
cl_kernel kernel;
// Initialises the command queue to the device
cl_command_queue queue;
cl_event event = NULL;

int err;

// This selects the device to be used for the OpenCL code, generally this is the GPU
cl_device_id create_device();
// This creates a context to run the kernel in
void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname);
// This builds the program from the source code on the source file
cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename);
// Creates a buffer then adds it to gpu called bufV
void setup_kernel_memory();
// Copies the kernel arguments to the kernel
void copy_kernel_args();
// Frees up the memory to prevent memory leaks from kernel
void free_memory();

void init(int *&A, int size);
void print(int *A, int size);

int main(int argc, char **argv)
{
    if (argc > 1) SZ = atoi(argv[1]);

    // Vector to be add
    // init(v, SZ);

    // Vectors to be multiplied
    init(v1, SZ * SZ);
    init(v2, SZ * SZ);
    v3 = (int *)malloc(sizeof(int) * SZ * SZ);

    // The global size is determined based on the size of the vector specified by atoi(argv[1]).
    // It represents the total number of work-items that will be processed by the kernel.
    size_t global[] = {(size_t)SZ, (size_t)SZ};

    //initial for add vector
    // print(v, SZ);

    std::cout << "-----------------------------" << std::endl;
    //initial for multiply vector
    print(v1, SZ * SZ);
    print(v2, SZ * SZ);
    std::cout << "-----------------------------" << std::endl;

    // setup_openCL_device_context_queue_kernel((char *)"./vector_ops.cl", (char *)"square_magnitude");
    setup_openCL_device_context_queue_kernel((char *)"./vector_ops.cl", (char *)"square_product");

    setup_kernel_memory();
    copy_kernel_args();

    /* `clEnqueueNDRangeKernel` enqueues a kernel for execution on a device.

     Parameters:
     - `cl_command_queue command_queue`: The queue where the kernel is queued.
     - `cl_kernel kernel`: The kernel to be executed.
     - `cl_uint work_dim`: Number of dimensions for global and local work-items.
     - `const size_t *global_work_offset`: Offset for global work-items.
     - `const size_t *global_work_size`: Number of global work-items.
     - `const size_t *local_work_size`: Work-group size.
     - `cl_uint num_events_in_wait_list`: Number of events in the wait list.
     - `const cl_event *event_wait_list`: Events that must complete before execution.
     - `cl_event *event`: Event object for tracking or waiting on kernel execution.
    */

    clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, NULL, 0, NULL, &event);
    clWaitForEvents(1, &event);

    /* `clEnqueueReadBuffer` is used to enqueue commands for reading from a buffer object to host memory.

     Parameters:
     - `cl_command_queue command_queue`: The queue where the read command will be added.
     - `cl_mem buffer`: The buffer to read from.
     - `cl_bool blocking_read`: Indicates if the read is blocking or non-blocking.
     - `size_t offset`: Byte offset within the buffer to start reading from.
     - `size_t size`: Number of bytes to read.
     - `void *ptr`: Pointer to host memory where data will be read into.
     - `cl_uint num_events_in_wait_list`: Number of events in the wait list.
     - `const cl_event *event_wait_list`: List of events that must complete before the read.
     - `cl_event *event`: Returns an event object for tracking or waiting on the read operation.
    */

    // clEnqueueReadBuffer(queue, bufV, CL_TRUE, 0, SZ * sizeof(int), &v[0], 0, NULL, NULL);
    clEnqueueReadBuffer(queue, bufV3, CL_TRUE, 0, SZ * SZ * sizeof(int), &v3[0], 0, NULL, NULL);

    //result vector
    // print(v, SZ);
    // std::cout << "-----------------------------" << std::endl;
    print(v3, SZ * SZ);
    std::cout << "-----------------------------" << std::endl;

    //frees memory for device, kernel, queue, etc.
    //you will need to modify this to free your own buffers
    free_memory();
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
        {                        //rows
            printf("%d ", A[i]); // print the cell value
        }
        printf(" ..... ");
        for (long i = size - 5; i < size; i++)
        {                        //rows
            printf("%d ", A[i]); // print the cell value
        }
    }
    else
    {
        for (long i = 0; i < size; i++)
        {                        //rows
            printf("%d ", A[i]); // print the cell value
        }
    }
    printf("\n----------------------------\n");
}

void free_memory()
{
    //free the buffers
    // clReleaseMemObject(bufV);

    clReleaseMemObject(bufV1);
    clReleaseMemObject(bufV2);
    clReleaseMemObject(bufV3);

    //free opencl objects
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseProgram(program);
    clReleaseContext(context);

    // free(v);

    free(v1);
    free(v2);
    free(v3);
}

void copy_kernel_args()
{
    /* `clSetKernelArg` sets the value for a specific argument of a kernel.

     Parameters:
     - `cl_kernel kernel`: The kernel object to update.
     - `cl_uint arg_index`: Index of the argument to set.
     - `size_t arg_size`: Size of the argument value in bytes.
     - `const void *arg_value`: Pointer to the argument value.
    */

    clSetKernelArg(kernel, 0, sizeof(int), (void *)&SZ);
    // clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&bufV);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&bufV1);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&bufV2);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&bufV3);

    if (err < 0)
    {
        perror("Couldn't create a kernel argument");
        printf("error = %d", err);
        exit(1);
    }
}

void setup_kernel_memory()
{
    /*
    clCreateBuffer is used to create a buffer object in OpenCL.
    It allocates memory for data storage and specifies how it can be accessed.

    Parameters:
    - cl_context context: The OpenCL context associated with the buffer.
    - cl_mem_flags flags: Memory access flags. Supported flags include:
        - CL_MEM_READ_WRITE
        - CL_MEM_WRITE_ONLY
        - CL_MEM_READ_ONLY
        - CL_MEM_ALLOC_HOST_PTR
        - CL_MEM_HOST_WRITE_ONLY
        - CL_MEM_HOST_READ_ONLY
        - CL_MEM_HOST_NO_ACCESS
        - CL_MEM_KERNEL_READ_AND_WRITE
        - CL_MEM_USE_HOST_PTR
    - size_t size: The size in bytes of the buffer's data storage.
    - void *host_ptr: A pointer to the host data that can be associated with the buffer.
    - cl_int *errcode_ret: A pointer to store the error code (if any).
    */
    // bufV = clCreateBuffer(context, CL_MEM_READ_WRITE, SZ * sizeof(int), NULL, NULL);
    bufV1 = clCreateBuffer(context, CL_MEM_READ_WRITE, SZ * SZ * sizeof(int), NULL, NULL);
    bufV2 = clCreateBuffer(context, CL_MEM_READ_WRITE, SZ * SZ * sizeof(int), NULL, NULL);
    bufV3 = clCreateBuffer(context, CL_MEM_READ_WRITE, SZ * SZ * sizeof(int), NULL, NULL);

    // Copy matrices to the GPU
    // clEnqueueWriteBuffer(queue, bufV, CL_TRUE, 0, SZ * sizeof(int), &v[0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, bufV1, CL_TRUE, 0, SZ * SZ * sizeof(int), &v1[0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, bufV2, CL_TRUE, 0, SZ * SZ * sizeof(int), &v2[0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, bufV3, CL_TRUE, 0, SZ * SZ * sizeof(int), &v3[0], 0, NULL, NULL);
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

cl_device_id create_device() {

   cl_platform_id platform;
   cl_device_id dev;
   int err;

   /* Identify a platform */
   err = clGetPlatformIDs(1, &platform, NULL);
   if(err < 0) {
      perror("Couldn't identify a platform");
      exit(1);
   } 

   // Access a device
   // GPU
   err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
   if(err == CL_DEVICE_NOT_FOUND) {
      // CPU
      printf("GPU not found\n");
      err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
   }
   if(err < 0) {
      perror("Couldn't access any devices");
      exit(1);   
   }

   return dev;
}