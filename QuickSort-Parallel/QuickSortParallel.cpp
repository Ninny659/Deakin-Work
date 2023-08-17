#include <iostream>
#include <random>
#include <utility>
#include <functional>
#include <chrono>
#include <pthread.h>

using namespace std;

static int N = 10;
const int NUM_THREAD = 4;

pthread_mutex_t lock;

// Arguments to use for both instances of quick sort
struct ArrayArgs
{
    int *a1;
    int minLength, maxLength;
};

// Generating a new array and assigning it with random values
int *unsortedArrayGenerator()
{
    int *array = new int[N];
    
    // Creating an array with rnd values between 1-100
    for (int i = 0; i < N; i++)
    {
        array[i] = rand() % 100;
    }

    return array;
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

int partition(int array[], int minLength, int maxLength) {
    
    // Using a mutex on code I want ran in sequential as it might cause errors as multiple threads run this code. 
    pthread_mutex_lock(&lock);
    int count = 0;

    // Count the number of elements smaller than or equal to the pivot
    for (int i = minLength + 1; i <= maxLength; i++) {
        if (array[i] <= array[minLength]) {
            count++;
        }
    }

    // Calculate the pivot's final index
    int pivotID = minLength + count;

    // Swap the pivot element to its final position
    swap(array[pivotID], array[minLength]);

    // Initialising variables for traversing the array
    int i = minLength;
    int j = maxLength;

    /* Partition the array into two parts: elements less than or equal to pivot on the left,
    and elements greater than pivot on the right
    */
    while (i < pivotID && j > pivotID) {
        // Find an element on the left side that is greater than the pivot
        while (array[i] <= array[minLength]) {
            i++;
        }

        // Find an element on the right side that is smaller than or equal to the pivot
        while (array[j] > array[minLength]) {
            j--;
        }

        // Swap the elements if i is to the left of pivot and j is to the right of pivot
        if (i < pivotID && j > pivotID) {
            swap(array[i++], array[j--]);
        }
    }

    pthread_mutex_unlock(&lock);
    return pivotID;
}

void *fastort(void *args)
{
    // Assigning passed through args to it a local struct args.
    ArrayArgs *fast_task = (ArrayArgs *)args;

    int *unsortedArr = fast_task->a1;
    int minLength = fast_task->minLength;
    int maxLength = fast_task->maxLength;

    // Simple edge case to determine if the array exsists and is greater than 1
    if (minLength < maxLength)
    {
        // Determining the pivot point. 
        int pivotID = partition(unsortedArr, minLength, maxLength);

        // Assigning left and right arguments to pass through the different threads
        ArrayArgs leftArgs = {unsortedArr, minLength, pivotID - 1};
        ArrayArgs rightArgs = {unsortedArr, pivotID + 1, maxLength};

        pthread_t leftThread, rightThread;

        // Creating and splitting the array in half, running both the left and right sides of the array.
        pthread_create(&leftThread, NULL, fastort, &leftArgs);
        pthread_create(&rightThread, NULL, fastort, &rightArgs);

        // Getting the threads to rejoin to the pool
        pthread_join(leftThread, NULL);
        pthread_join(rightThread, NULL);
    }

    return NULL;
}

double fastortHelper()
{
    int *fastArray = unsortedArrayGenerator();

    double executionTime_fastification = timerFunction([&]() {
        // Starting up the arguments for the main thread.
        ArrayArgs fast_task = {fastArray, 0, N - 1};
        pthread_t mainThread;

        // Running the sorting array on the main thread.
        pthread_create(&mainThread, NULL, fastort, &fast_task);
        pthread_join(mainThread, NULL);
    });

    // Removing array from memory
    delete[] fastArray;

    return executionTime_fastification;
}

void *slort(void *args)
{
    ArrayArgs *slow_task = (ArrayArgs *)args;

    int *unsortedArr = slow_task->a1;
    int minLength = slow_task->minLength;
    int maxLength = slow_task->maxLength;

    // Simple edge case to determine is array exsists and is greater than 1
    if (minLength < maxLength)
    {
        /*
        Similar to the parallel task, we split the array in half and run the sort on left and right portions of the array.
        Unlike the parallel task we are recursively running the function and will run all the left operations before running the right operation
        In theory running this would be slower compared to the parallel task as we are only running one thread compared to multiple threads.  
        */ 
        int pivotID = partition(unsortedArr, minLength, maxLength);

        ArrayArgs leftArgs = {unsortedArr, minLength, pivotID - 1};
        ArrayArgs rightArgs = {unsortedArr, pivotID + 1, maxLength};

        slort(&leftArgs);

        slort(&rightArgs);
    }
    
    return NULL;
}

double slortHelper()
{
    int *slowArray = unsortedArrayGenerator();

    double executionTime_slowification = timerFunction([&]() {
        // Assiging the needed struct to the sequential function while starting the timer. 
        ArrayArgs slow_task = {slowArray, 0, N - 1};
        slort(&slow_task);
    });

    // Clearing the memory used for the array.
    delete[] slowArray;

    return executionTime_slowification;
}

int main()
{
    const int RUN_TIME = 4;

    double fastortMean = 0;
    double slortMean = 0;

    /*
    A simple function to time the average of RUN_TIME for a multiple of the functions.
    This allows us to run each function for x (RUN_TIME) amount of timeas and take the average time of the runs.
    We can then increase the size of the array by multiplying N by z (Where z is an integer).
    */
    for (int i = 0; i <= 4; i++)
    {
        fastortMean = 0;
        slortMean = 0;

        for (int j = 0; j <= RUN_TIME; j++)
        {
            slortMean += slortHelper();
            fastortMean += fastortHelper();
        }

        slortMean /= RUN_TIME;
        fastortMean /= RUN_TIME;

        // Printing statements to display run time. 
        std::cout << "--------------------------------------------------------------\n" << "Execution Time for " << N << " elements (Fastort): " << fastortMean << " seconds (Mean) \n" << std::endl;
        std::cout << "--------------------------------------------------------------\n" << "Execution Time for " << N << " elements (Slort): " << slortMean << " seconds (Mean) \n" << std::endl;

        N *= 10;
    }

    return 0;
}
