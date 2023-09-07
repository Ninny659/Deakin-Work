#include <iostream>
#include <random>
#include <utility>
#include <functional>
#include <chrono>
#include <pthread.h>
#include <omp.h>
#include <opencl.hpp> 
#include <fstream>

using namespace std;

// Matrix size -> anything past 100,000 is about 13gb of memory usage
static int N = 1;

// Number of threads being used
const int NUM_THREAD = 30;

// Min and Max for array rands
const int minRnd = 5;
const int maxRnd = 12;

struct MatrixArgs
{
    int startRow, endRow;
    int **m1, **m2, **m3;
};

// Super inefficent. Caused issues
// int rndNumber()
// {
//     std::random_device rnd;
//     std::mt19937 numGen(rnd());
//     std::uniform_int_distribution<int> distribution(minRnd, maxRnd);
//
//     return distribution(numGen);
// }

// A function to generate and fill matrixs' with random values. If true then the resulting matrix will be filled with 0's.
int **matrixGenerator(bool resultMatrix)
{
    int i, j;

    // Defining and allocating memory to the matrix to the size of N.
    int **matrix = (int **)malloc(N * sizeof(int *));

    for (i = 0; i < N; i++)
    {
        matrix[i] = (int *)malloc(N * sizeof(int));
    }

    // Running through each position of the Matrix and defining it as rand() % 100
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            if (resultMatrix != true)
                matrix[i][j] = rand() % 100;
            // If it is a result matrix, then 0 is inserted instead of rand()
            else
                matrix[i][j] = 0;
        }
    }

    return matrix;
}

double timerFunction(std::function<void(void)> timedFunc)
{
    // Start a clock when we call the function
    auto start = std::chrono::high_resolution_clock::now();

    // Execute the function
    timedFunc();

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end - start;

    // Calculate the time taken from the start of the clock to the end and return to helper functions.
    return elapsed.count();
}

// Free allocated memory from passed refrences.
void freeMemory(int **matrix1, int **matrix2, int **matrix_result)
{
    for (int i = 0; i < N; i++)
    {
        // Running through each matrix element and defining it as usable space.
        free(matrix1[i]);
        free(matrix2[i]);
        free(matrix_result[i]);
    }

    free(matrix1);
    free(matrix2);
    free(matrix_result);
}

// Our slow non-parallel matrix multiplication implmentation
void *slowtication(void *args)
{
    /*
    Creating a refrence to the struct that holds all the values needed for matrix multiplication.
    MatrixArgs is a shared resources for all of the matrix multiplication instances to reduce code complexity.
    */
    struct MatrixArgs *slow_task = (struct MatrixArgs *)args;

    // Normal matrix multiplication where the tasks is defined from the starting element [0][0] to the end [N][N]
    int i, j, k;
    for (i = slow_task->startRow; i <= slow_task->endRow; i++)
    {
        for (j = 0; j < N; j++)
        {
            for (k = 0; k < N; k++)
            {
                // Assiging matrix3 to be the mult of matrix1 and matrix2
                slow_task->m3[i][j] += slow_task->m1[i][k] * slow_task->m2[k][j];
            }
        }
    }

    // return NULL as all of our values are save to the struct refrence.
    return NULL;
}

// Defining helper function to reduce code in the main() func
double slowticationHelper()
{
    // Declaring matrixs' to be generated with random values
    int **matrix1 = matrixGenerator(false);
    int **matrix2 = matrixGenerator(false);

    // Declaring matrixs' to be generated with 0's as it's values
    int **slow_result = matrixGenerator(true);

    // Declaring a double timer result (Seconds) to print later
    double executionTime_slowtication = timerFunction([&]()
    {
        struct MatrixArgs slow_task;
        // Assining all of our values to the struct to be passed by refrence
        slow_task.m1 = matrix1;
        slow_task.m2 = matrix2;
        slow_task.m3 = slow_result;
        slow_task.startRow = 0;
        slow_task.endRow = N - 1;
        slowtication((void *)&slow_task); });

    // Printing the time taken to execute the function
    //std::cout << "Execution Time (Slowtication): " << executionTime_slowtication << " seconds" << std::endl;

    // Freeing used memory to be utalised again.
    freeMemory(matrix1, matrix2, slow_result);

    return executionTime_slowtication;
}

void *fastication(void *args)
{
    MatrixArgs *fast_task = (MatrixArgs *)args;

    /*
    Perform multiplication in the assigned chunk of rows and columns.
    Instead of running through every element the threds divide the matrix into smaller sub-matrixs',
    to compute the mult faster and more efficent.
    This substantially cut's down on the time taken but increases the overhead of calculation.
    */

    for (int i = fast_task->startRow; i <= fast_task->endRow; i++)
    {
        for (int j = fast_task->startRow; j < fast_task->endRow; j++)
        {
            for (int k = fast_task->startRow; k < fast_task->endRow; k++)
            {
                fast_task->m3[i][j] += fast_task->m1[i][k] * fast_task->m2[k][j];
            }
        }
    }

    // return NULL as all of our values are save to the struct refrence.
    return NULL;
}

double fasticationHelper()
{

    // Declaring matrixs' to be generated with random values
    int **matrix1 = matrixGenerator(false);
    int **matrix2 = matrixGenerator(false);

    // Declaring matrixs' to be generated with 0's as it's values
    int **fast_result = matrixGenerator(true);

    double executionTime_fastification = timerFunction([&]()
    {

        // Defining and creating threads based off of the const NUM_THREAD and chunking the matrix size [N] by the number of threads
        pthread_t fast_threads[NUM_THREAD];
        MatrixArgs fast_tasks[NUM_THREAD];
        int Chunk_size = N / NUM_THREAD;

        // Assigning the different threads a struct with differing starting start and end positions.
        for (size_t i = 0; i < NUM_THREAD; i++) { 
            fast_tasks[i].m1 = matrix1;
            fast_tasks[i].m2 = matrix2;
            fast_tasks[i].m3 = fast_result;
            fast_tasks[i].startRow = i * Chunk_size;
            fast_tasks[i].endRow = ((i + 1) * Chunk_size) - 1;

            // Creating these threads and assiging them a function to run.
            pthread_create(&fast_threads[i], NULL, fastication, (void *)&fast_tasks[i]);
        }

        // Awaiting threads to rejoin the pool of avaliable threads.
        for (size_t i = 0; i < NUM_THREAD; i++) {
            pthread_join(fast_threads[i], NULL);
        } });

    // Printing the time taken to execute the function
    //std::cout << "Execution Time (Fastification): " << executionTime_fastification << " seconds" << std::endl;

    // Freeing used memory to be utalised again.
    freeMemory(matrix1, matrix2, fast_result);

    return executionTime_fastification;
}

void *blazingtication(void *args)
{

    struct MatrixArgs *blazing_task = (struct MatrixArgs *)args;

// No need to set up parallism here, the libary will dynamically handle the task aswell as thread assignment.
#pragma omp parallel for reduction(+: res)
    for (int i = blazing_task->startRow; i <= blazing_task->endRow; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                // Assiging it to a int to so OMP knows what it needs to optimise for. 
                int res = blazing_task->m1[i][k] * blazing_task->m2[k][j];
                blazing_task->m3[i][j] = res;
            }
        }
    }

    // return NULL as all of our values are save to the struct refrence.
    return NULL;
}

double blazingticationHelper()
{
    // Declaring matrixs' to be generated with random values
    int **matrix1 = matrixGenerator(false);
    int **matrix2 = matrixGenerator(false);

    // Declaring matrixs' to be generated with 0's as it's values
    int **blazing_result = matrixGenerator(true);

    // Declaring a double timer result (Seconds) to print later
    double executionTime_blazingtication = timerFunction([&]()
    {
        struct MatrixArgs blazing_task;
        // Assining all of our values to the struct to be passed by refrence
        blazing_task.m1 = matrix1;
        blazing_task.m2 = matrix2;
        blazing_task.m3 = blazing_result;
        blazing_task.startRow = 0;
        blazing_task.endRow = N - 1;
        blazingtication((void *)&blazing_task); 
    });

    // Printing the time taken to execute the function
    //std::cout << "Execution Time (Blazingtication): " << executionTime_blazingtication << " seconds" << std::endl;

    // Freeing used memory to be utalised again.
    freeMemory(matrix1, matrix2, blazing_result);

    return executionTime_blazingtication;
}

std::string matrixMultiplicationKernelSourceReader(const char *filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open kernel file");
    }
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

double matrixMultiplicationOpenCLHelper()
{
    // Declaring matrixs' to be generated with random values
    int **matrix1 = matrixGenerator(false);
    int **matrix2 = matrixGenerator(false);

    // Declaring matrixs' to be generated with 0's as it's values
    int **multiplication_result = matrixGenerator(true);

    double executionTime_multiplication = timerFunction([&]()
    {


    });   

    freeMemory(matrix1, matrix2, multiplication_result);
    return executionTime_multiplication;
}

int main()
{
    // Helper functions to retain cleanliness in the main function. Can expand to further testing and benchmarks from here.
    const int RUN_TIME = 4;
    double slowMean, fastMean, blazeMean, multiplicationMean = 0;

    for(int i = 0; i < 5; i++)
    {
        for(int j = 0; j < RUN_TIME; j++){
            slowMean += slowticationHelper();
            fastMean += fasticationHelper();
            blazeMean += blazingticationHelper();
            multiplicationMean += matrixMultiplicationOpenCLHelper();

        }

        slowMean /= RUN_TIME;
        fastMean /= RUN_TIME;
        blazeMean /= RUN_TIME;
        multiplicationMean /= RUN_TIME;

        std::cout << "--------------------------------------------------------------\n"
        "Execution Time for " << N << " elements (Slowtication): " << slowMean << " seconds (Mean) \n" 
        "Execution Time for " << N << " elements (Fastication): " << fastMean << " seconds (Mean) \n" 
        "Execution Time for " << N << " elements (Blazingtication): " << blazeMean << " seconds (Mean) \n" 
        "Execution Time for " << N << " elements (MatrixMultiplicationOpenCL): " << multiplicationMean << " seconds (Mean) \n"
        
        
        << std::endl;
        N *=  10;
    }


    return 0;
}