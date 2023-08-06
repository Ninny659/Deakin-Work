#include <iostream>
#include <random>
#include <utility>
#include <functional>
#include <chrono>
#include <pthread.h>

using namespace std;

// Matrix size
const int N = 10000;

// Number of threads being used
const int NUM_THREAD = 6;

// Min and Max for array rands
const int minRnd = 5;
const int maxRnd = 12;

struct SlowticationArgs
{
    int startRow, endRow;
    int **m1, **m2, **m3;
};

struct FastificationArgs
{
    int startRow, endRow;
    int **m1, **m2, **m3;
};

int rndNumber()
{
    std::random_device rnd; 
    std::mt19937 numGen(rnd()); 
    std::uniform_int_distribution<int> distribution(minRnd, maxRnd); 

    return distribution(numGen); 
}

int **matrixGenerator(bool resultMatrix)
{
    int i, j;

    int **matrix = (int **)malloc(N * sizeof(int *));

    for (i = 0; i < N; i++)
    {
        matrix[i] = (int *)malloc(N * sizeof(int));
    }

    // Initialize the matrix with random values

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            if (resultMatrix != true) matrix[i][j] = rand();
            else matrix[i][j] = 0;
        }
    }

    return matrix;
}

double timerFunction(std::function<void(void)> func)
{
    auto start = std::chrono::high_resolution_clock::now();

    func();

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end - start;

    return elapsed.count();
}

void freeMemory(int** matrix1, int** matrix2, int** matrix_result)
{
    // Free allocated memory
    for (int i = 0; i < N; i++)
    {
        free(matrix1[i]);
        free(matrix2[i]);
        free(matrix_result[i]);
    }
    free(matrix1);
    free(matrix2);
    free(matrix_result);
}

void *slowtication(void *args)
{
    struct SlowticationArgs *slow_task = (struct SlowticationArgs *)args;

    int i, j, k;
    for (i = slow_task->startRow; i <= slow_task->endRow; i++)
    {
        for (j = 0; j < N; j++)
        {

            slow_task->m3[i][j] = 0;

            for (k = 0; k < N; k++)
            {
                slow_task->m3[i][j] += slow_task->m1[i][k] * slow_task->m2[k][j];
            }
        }
    }

    return NULL;
}

void slowticationHelper()
{
    int **matrix1 = matrixGenerator(false);
    int **matrix2 = matrixGenerator(false);

    int **slow_result = matrixGenerator(true);

    double executionTime_slowtication = timerFunction([&]() {
        struct SlowticationArgs slow_task;
        slow_task.m1 = matrix1;
        slow_task.m2 = matrix2;
        slow_task.m3 = slow_result;
        slow_task.startRow = 0;
        slow_task.endRow = N - 1;
        slowtication((void *)&slow_task); 
    });

    std::cout << "Execution Time (Slowtication): " << executionTime_slowtication << " seconds" << std::endl;

    freeMemory(matrix1, matrix2, slow_result);
}

void *fastication(void *args)
{
    FastificationArgs *fast_task = (FastificationArgs *)args;

    // Perform multiplication in the assigned chunk of rows
    for (int i = fast_task->startRow; i <= fast_task->endRow; i++)
    {
        for (int j = 0; j < N; j++)
        {
            fast_task->m3[i][j] = 0;
            for (int k = 0; k < N; k++)
            {
                fast_task->m3[i][j] += fast_task->m1[i][k] * fast_task->m2[k][j];
            }
        }
    }

    return NULL;
}

void fasticationHelper()
{

    int **matrix1 = matrixGenerator(false);
    int **matrix2 = matrixGenerator(false);

    int **fast_result = matrixGenerator(true);

    double executionTime_fastification = timerFunction([&]() {

        pthread_t fast_threads[NUM_THREAD];
        FastificationArgs fast_tasks[NUM_THREAD];
        int Chunk_size = N / NUM_THREAD;

        for (size_t i = 0; i < NUM_THREAD; i++) {
            fast_tasks[i].m1 = matrix1;
            fast_tasks[i].m2 = matrix2;
            fast_tasks[i].m3 = fast_result;
            fast_tasks[i].startRow = i * Chunk_size;
            fast_tasks[i].endRow = ((i + 1) * Chunk_size) - 1;

            pthread_create(&fast_threads[i], NULL, fastication, (void *)&fast_tasks[i]);
        }

        for (size_t i = 0; i < NUM_THREAD; i++) {
            pthread_join(fast_threads[i], NULL);
        } 
    });
    
    std::cout << "Execution Time (Fastification): " << executionTime_fastification << " seconds" << std::endl;

    freeMemory(matrix1, matrix2, fast_result);
}

int main()
{
    slowticationHelper();

    fasticationHelper();

    return 0;
}