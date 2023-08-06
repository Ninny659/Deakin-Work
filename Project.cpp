#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>

using namespace std::chrono;
using namespace std;

// Constant variables that we don't change during run time.
#define NUM_THREAD 5

const int vectorSize = 100000000;

struct RndVectorRefrence
{
    int *v;
    int startPosition, endPosition;
};

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

int main()
{
    pthread_t random_threads[NUM_THREAD];
    pthread_t add_threads[NUM_THREAD];

    srand(time(0));

    int *v1, *v2, *v3;

    // Starting our timer and setting it to the timer_start variable.
    auto timer_start = high_resolution_clock::now();

    // Setting up a vector of vectorSize - int vectorSize
    v1 = (int *)malloc(vectorSize * sizeof(int *));
    v2 = (int *)malloc(vectorSize * sizeof(int *));
    v3 = (int *)malloc(vectorSize * sizeof(int *));

    // Assigning half of our threads to a specific task (We currently have two for loops running at the same time)
    int Chunk_size = vectorSize / NUM_THREAD / 2;

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
    Chunk_size = vectorSize / NUM_THREAD;

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

    // Retriving the time value of stop - timer_start together, for result duration.
    auto duration = duration_cast<microseconds>(timer_stop - timer_start);

    // Displaying the time taken to terminal.
    cout << "Time taken by function: "
         << duration.count() << " microseconds" << endl;

    return 0;
}