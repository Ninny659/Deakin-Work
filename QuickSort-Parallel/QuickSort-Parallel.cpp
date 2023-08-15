#include <iostream>
#include <random>
#include <utility>
#include <functional>
#include <chrono>
#include <pthread.h>

using namespace std;

static int N = 5;
const int NUM_THREAD = 10;

struct ArrayArgs
{
    int *a1;
    int minLength, maxLength;
};

void swap(int &a, int &b)
{
    int temp = a;
    a = b;
    b = temp;
}

int *unsortedArrayGenerator()
{
    int *array = new int[N];
    
    for (int i = 0; i < N; i++)
    {
        array[i] = rand() % 100;
    }

    return array;
}

double timerFunction(std::function<void(void)> timedFunc)
{
    auto start = std::chrono::high_resolution_clock::now();
    timedFunc();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    return elapsed.count();
}

int partition(int array[], int low, int high)
{
    int pivot = array[high];
    int i = (low - 1);

    for (int j = low; j < high; j++)
    {
        if (array[j] <= pivot)
        {
            i++;
            swap(array[i], array[j]);
        }
    }
    swap(array[i + 1], array[high]);
    return (i + 1);
}

void *fastort(void *args)
{
    ArrayArgs *fast_task = (ArrayArgs *)args;

    int *unsortedArr = fast_task->a1;
    int minLength = fast_task->minLength;
    int maxLength = fast_task->maxLength;

    if (minLength < maxLength)
    {
        int pivotIdx = partition(unsortedArr, minLength, maxLength);

        ArrayArgs leftArgs = {unsortedArr, minLength, pivotIdx - 1};
        ArrayArgs rightArgs = {unsortedArr, pivotIdx + 1, maxLength};

        pthread_t leftThread, rightThread;

        pthread_create(&leftThread, NULL, fastort, &leftArgs);
        pthread_create(&rightThread, NULL, fastort, &rightArgs);

        pthread_join(leftThread, NULL);
        pthread_join(rightThread, NULL);
    }

    return NULL;
}

double fastortHelper()
{
    int *array1 = unsortedArrayGenerator();
    int *maxSize = new int[N];

    for (int i = 0; i < N; ++i)
    {
        maxSize[i] = i + 1;
    }

    double executionTime_fastification = timerFunction([&]() {
        ArrayArgs fast_task = {array1, 0, N - 1};
        pthread_t mainThread;
        pthread_create(&mainThread, NULL, fastort, &fast_task);
        pthread_join(mainThread, NULL);
    });

    delete[] array1;
    delete[] maxSize;

    return executionTime_fastification;
}

int main()
{
    const int RUN_TIME = 4;
    double fastortMean = 0;

    for (int i = 0; i <= 5; i++)
    {
        for (int j = 0; j < RUN_TIME; j++)
        {
            fastortMean += fastortHelper();
        }

        fastortMean /= RUN_TIME;

        std::cout << "--------------------------------------------------------------\n"
                  << "Execution Time for " << N << " elements (Fastort): " << fastortMean << " seconds (Mean) \n"
                  << std::endl;
        N *= 10;
    }

    return 0;
}
