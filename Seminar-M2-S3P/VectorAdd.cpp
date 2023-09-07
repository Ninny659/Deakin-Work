#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <pthread.h>

using namespace std::chrono;
using namespace std;

// const variables we don't want to change in run time
const int NUM_THREAD = 6;
static int N = 10;

struct RndVectorRefrence
{
    int *v;
    int startPosition, endPosition;
};

// OMP implementation ---------------------------------------------------------------------------------------------------------------
void randomOMPVector(int vector[], int size)
{
#pragma omp parallel default(none)
    #pragma omp for
    for (int i = 0; i < size; i++)
    {
        // Adding rand value to the vector.
        vector[i] = rand() % 100;
    }
}

double ompVectorAddition()
{
    srand(time(0));

    int *v1, *v2, *v3;

    // Starting our timer and setting it to the timer_start variable.
    auto start = high_resolution_clock::now();

    // Adding vector1 to vector2 to the result of vector3
#pragma omp parallel default(none) shared(N, v3) private(v1, v2) reduction(+:totalV3)
    // Setting up a vector of N - int N
    v1 = (int *)malloc(N * sizeof(int *));
    v2 = (int *)malloc(N * sizeof(int *));
    v3 = (int *)malloc(N * sizeof(int *));

    randomOMPVector(v1, N);
    randomOMPVector(v2, N);

    int totalV3, total = 0;

    #pragma omp for schedule(guided, N)
    for (int i = 0; i < N; i++)
    {
        v3[i] = v1[i] + v2[i];
    }

    #pragma omp for 
    for (int i = 0; i < N; i++) 
    { 
        totalV3 += v3[i]; 
    } 

    #pragma omp critical 
    {
        total += totalV3;
    }

#pragma omp critical
{
    // Stopping the timer to measure affectivness of our implmentation.
    auto timer_stop = high_resolution_clock::now();

    std::chrono::duration<double> elapsed = timer_stop - start;
    return elapsed.count();
}
}

// pThread implementation ---------------------------------------------------------------------------------------------------------------
struct AddVectorRefrence
{
    int *v1, *v2, *v3;
    int startPosition, endPosition;
};

void *create_randomVector(void *args)
{
    RndVectorRefrence *rnd_vectortaskTask = ((struct RndVectorRefrence *)args);

    for (int i = rnd_vectortaskTask->startPosition; i < rnd_vectortaskTask->endPosition; i++)
    {
        // Adding rand value to the vector.
        rnd_vectortaskTask->v[i] = rand() % 100;
    }

    return 0;
}

void *adding_vector(void *args)
{
    AddVectorRefrence *add_vectorTask = ((struct AddVectorRefrence *)args);

    for (int i = add_vectorTask->startPosition; i < add_vectorTask->endPosition; i++)
    {
        // Adding vector1 to vector2 to the result of vector3
        add_vectorTask->v3[i] = add_vectorTask->v1[i] + add_vectorTask->v2[i];
    }

    return 0;
}

double pthreadVectorAddition()
{
    pthread_t random_threads[NUM_THREAD];
    pthread_t add_threads[NUM_THREAD];

    srand(time(0));

    int *v1, *v2, *v3;

    // Starting our timer and setting it to the timer_start variable.
    auto timer_start = high_resolution_clock::now();

    // Setting up a vector of N - int N
    v1 = (int *)malloc(N * sizeof(int *));
    v2 = (int *)malloc(N * sizeof(int *));
    v3 = (int *)malloc(N * sizeof(int *));

    // Assigning half of our threads to a specific task (We currently have two for loops running at the same time)
    int Chunk_size = N / NUM_THREAD / 2;

    struct RndVectorRefrence *random_task = (struct RndVectorRefrence *)malloc(sizeof(struct RndVectorRefrence));

    // Creating assigning random values to vector1 and vector2
    for (size_t i = 0; i < NUM_THREAD / 2; i++)
    {
        // Assigning our values to the struct to access in the called function
        random_task->v = v1;
        // Defining the start and end position for all of the assigned threads
        random_task->startPosition = i * Chunk_size;
        random_task->endPosition = ((i + 1) * Chunk_size) - 1;

        // Creating a thread to run create_randomVector
        pthread_create(&random_threads[i], NULL, create_randomVector, (void *)random_task);
    }

    for (size_t i = NUM_THREAD / 2; i < NUM_THREAD; i++)
    {
        // See first for loop on explanation.
        random_task->v = v2;
        random_task->startPosition = i * Chunk_size;
        random_task->endPosition = ((i + 1) * Chunk_size) - 1;

        // Creating a thread to run create_randomVector
        pthread_create(&random_threads[i], NULL, create_randomVector, (void *)random_task);
    }

    for (size_t i = 0; i < NUM_THREAD; i++)
    {
        // Awaiting all of the threads to finish their assigned function and rejoin the thead pool.
        pthread_join(random_threads[i], NULL);
    }

    // Assigning all of our threads to complete this task.
    Chunk_size = N / NUM_THREAD;

    // Iterating and adding the values of V1 and V2 together for result V3
    for (size_t i = 0; i < NUM_THREAD; i++)
    {
        struct AddVectorRefrence *add_task = (struct AddVectorRefrence *)malloc(sizeof(struct AddVectorRefrence));

        // Assigning the values to the previously declared struct to be accessed in the called function
        add_task->v1 = v1;
        add_task->v2 = v2;
        add_task->v3 = v3;
        add_task->startPosition = i * Chunk_size;
        add_task->endPosition = ((i + 1) * Chunk_size) - 1;

        // Assigning all of the threads to different chunks of the vector
        pthread_create(&add_threads[i], NULL, adding_vector, (void *)add_task);
    }

    for (size_t i = 0; i < NUM_THREAD; i++)
    {
        // Awaiting all of the threads to finish their assigned function and rejoin the thead pool.
        pthread_join(random_threads[i], NULL);
    }

    // Stopping the timer to measure affectivness of our implmentation.
    auto timer_stop = high_resolution_clock::now();

    std::chrono::duration<double> elapsed = timer_stop - timer_start;
    return elapsed.count();
}

// Sequential implementation ---------------------------------------------------------------------------------------------------------------

void randomSeqVector(int vector[], int size)
{
    for (int i = 0; i < size; i++)
    {
        // Adding rand value to the vector.
        vector[i] = rand() % 100;
    }
}

double sequentialAddition()
{
    srand(time(0));

    int *v1, *v2, *v3;

    // Starting our timer and setting it to the timer_start variable.
    auto start = high_resolution_clock::now();

    // Setting up a vector of N - int N
    v1 = (int *)malloc(N * sizeof(int *));
    v2 = (int *)malloc(N * sizeof(int *));
    v3 = (int *)malloc(N * sizeof(int *));

    randomOMPVector(v1, N);
    randomOMPVector(v2, N);

    // Adding vector1 to vector2 to the result of vector3
    for (int i = 0; i < N; i++)
    {
        v3[i] = v1[i] + v2[i];
    }


    // Stopping the timer to measure affectivness of our implmentation.
    auto timer_stop = high_resolution_clock::now();

    // Retriving the time value of stop - timer_start together, for result duration.
    auto duration = duration_cast<microseconds>(timer_stop - start);

    std::chrono::duration<double> elapsed = timer_stop - start;
    return elapsed.count();
}

int main()
{
    const int RUN_TIME = 5;
    double seqMean, pthreadMean, ompMean = 0;

    for(int i = 0; i <= 7; i++)
    {
        for(int j = 0; j < RUN_TIME; j++){
            seqMean += sequentialAddition();
            pthreadMean += pthreadVectorAddition();
            ompMean += ompVectorAddition();
        }

        seqMean /= RUN_TIME;
        pthreadMean /= RUN_TIME;
        ompMean /= RUN_TIME;

        std::cout << "--------------------------------------------------------------\n"
        "Execution Time for " << N << " elements (SequentialAddition): " << seqMean << " seconds (Mean) \n" 
        "Execution Time for " << N << " elements (pThreadVectorAddition): " << pthreadMean << " seconds (Mean) \n" 
        "Execution Time for " << N << " elements (Guided Schedule) (OMPVectorAddition): " << ompMean << " seconds (Mean) \n" 
        
        
        << std::endl;
        N *=  10;
    }
}