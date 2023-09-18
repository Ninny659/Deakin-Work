#include <iostream>
#include <random>
#include <utility>
#include <functional>
#include <chrono>
#include <iostream>
#include <mpi.h>

using namespace std;

const int N = 10000;

// Function to swap two elements
void swap(int* arr, int i, int j)
{
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

// Function to check if an array is sorted
void checkSortedArray(int* arr)
{
    for (int i = 0; i < N - 1; i++) {
        if (arr[i] > arr[i + 1]) {
            cout << "Array is not sorted" << endl;
            return;
        }
    }
    cout << "Array is sorted" << endl;
}

// Function to generate an array with random values
int* unsortedArrayGenerator()
{
    int* unsortedArray = (int*)malloc(N * sizeof(int));

    srand(time(NULL));
    for (int i = 0; i < N; i++) {
        unsortedArray[i] = rand() % 255;
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

// Function to sort an array using quick sort
void MPIsort(int* arr, int minLength, int maxLength)
{
    /* This is a quick sort, with swapping and partitioning.
    This code was expanded upon in the Sequential and Parallel versions of this code,
    please refer to those for more information.
    */
    int pivot, index;

    if (maxLength <= 1)
        return;

    pivot = arr[minLength + maxLength / 2];
    swap(arr, minLength, minLength + maxLength / 2);

    index = minLength;

    for (int i = minLength + 1; i < minLength + maxLength; i++) {
        if (arr[i] < pivot) {
            index++;
            swap(arr, i, index);
        }
    }

    swap(arr, minLength, index);

    MPIsort(arr, minLength, index - minLength);
    MPIsort(arr, index + 1, minLength + maxLength - index - 1);
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
void fastort(int* unsortedArray,int argc, char* argv[])
{
    // Initialize MPI
    int numProcs, rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Calculate the size of each chunk
    int localSize = N / numProcs;

    int* localChunk = (int*)malloc(localSize * sizeof(int));

    // Scatter the array into chunks, and sort each chunk. Size is localSize.
    MPI_Scatter(unsortedArray, localSize, MPI_INT, localChunk, localSize, MPI_INT, 0, MPI_COMM_WORLD);

    // localChunkSize is the size of the chunk that each process has.
    int localChunkSize;
    if (N >= localSize * (rank + 1)) localChunkSize = localSize;
    else localChunkSize = N - localSize * rank;

    // Debug print
    // cout << "\nRank " << rank << " has chunk: ";
    // for(int i = 0; i < localSize; i++)
    // {
    //     cout << localChunk[i] << " ";
    // }
    // std::cout << "\nLength of localChunk: " << localChunkSize << std::endl;

    // Sort the local chunk on each process
    MPIsort(localChunk, 0, localChunkSize);

    // Merge the sorted chunks
    for (int i = 1; i < numProcs; i = 2 * i) {
        if (rank % (2 * i) != 0) {
            // If the process is not the first process in the pair, send the localChunk to the process with rank - i.
            MPI_Send(localChunk, localChunkSize, MPI_INT, rank - i, 0, MPI_COMM_WORLD);
            break;
        }

        // If the process is the first process in the pair, receive the localChunk from the process with rank + i.
        if (rank + i < numProcs) {

            int receivedChunkSize;

            // If the process with rank + i has a chunk of size localSize, then the received chunk size is localSize * i.
            if(N >= localSize * (rank + 2 * i)) receivedChunkSize = localSize * i;
            else receivedChunkSize = N - localSize * (rank + i);

            // Declare a receivedChunk array of size receivedChunkSize
            int* receivedChunk = (int*)malloc(receivedChunkSize * sizeof(int));

            // Receive the chunk from the process with rank + i
            MPI_Recv(receivedChunk, receivedChunkSize, MPI_INT, rank + i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Merge the received chunks into localChunk
            localChunk = merge(localChunk, localChunkSize, receivedChunk, receivedChunkSize);

            // Increase the size of localChunk by receivedChunkSize
            localChunkSize += receivedChunkSize;

            // Debug print
            // cout << "Rank " << rank << " received chunk from rank " << rank + i << endl;
            // for(int i = 0; i < receivedChunkSize; i++)
            // {
            //     cout << receivedChunk[i] << " ";
            // }

            // Free the receivedChunk array
            free(receivedChunk);
        }
    }

    MPI_Finalize();

    // Check if the array is sorted
    if (rank == 0)
    {
        checkSortedArray(localChunk);
        // for(int i = 0; i < N; i++)
        // {
        //     cout << localChunk[i] << " ";
        // }
    }

    free(localChunk);
}

// Helper function to run fastort
double fastortHelper()
{
    int* unsortedArray = unsortedArrayGenerator();

    double executionTime_fastification = timerFunction([&]() {
        fastort(unsortedArray, 0, NULL);
    });

    // Removing array from memory
    free(unsortedArray);

    return executionTime_fastification;
}

// Main function
int main()
{
    double executionTime_fastification = 0;
    executionTime_fastification = fastortHelper();

    std::cout << "\nExecution Time for " << N << " elements (Fastort): " << executionTime_fastification << " microseconds \n" << std::endl;
    return 0;
}