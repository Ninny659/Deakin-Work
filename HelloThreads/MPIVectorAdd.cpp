#include <iostream>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <mpi.h>

using namespace std::chrono;

void randomVector(int vector[], int size) {
    for (int i = 0; i < size; i++) {
        // Adding rand value to the vector.
        vector[i] = rand() % 100;
    }
}

int main(int argc, char** argv) {
    srand(time(0)); // Seed with rank-dependent value

    // Setting up local size for each process
    unsigned long size = 100000000;
    long long localSum = 0;
    int numProcs, rank, nameLen, tag = 1;
    int totalSum = 0;
    char name[MPI_MAX_PROCESSOR_NAME];

    // Initialize MPI
    MPI_Status status;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(name, &nameLen);

    // Setting up local and global vectors for each process and head
    int *V1 = (int *) malloc(size * sizeof(int *));
    int *V2 = (int *) malloc(size * sizeof(int *));
    int *V3 = (int *) malloc(size * sizeof(int *));

    int localSize = size / numProcs;

    int *localV1 = (int *) malloc(localSize * sizeof(int *));
    int *localV2 = (int *) malloc(localSize * sizeof(int *));
    int *localV3 = (int *) malloc(localSize * sizeof(int *));

    // Setting up vectors for head process
    if (rank == 0) {
        randomVector(V1, size);
        randomVector(V2, size);
    }

    auto start = high_resolution_clock::now();

    // Scatter v1 and v2 to all processes
    MPI_Scatter(V1, localSize, MPI_INT, localV1, localSize, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(V2, localSize, MPI_INT, localV2, localSize, MPI_INT, 0, MPI_COMM_WORLD);

    // Perform local addition and add sum to local sum
    for (size_t i = 0; i < localSize; i++) {
        localV3[i] = localV1[i] + localV2[i];
        localSum += localV3[i];
    }

    // Gather local sums to head (rank 0)
    MPI_Gather(localV3, localSize, MPI_INT, V3, localSize, MPI_INT, 0, MPI_COMM_WORLD);
    // Reduce all local sums to total sum
    MPI_Reduce(&localSum, &totalSum, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);


    if (rank == 0) {
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop - start);
        std::cout << "Time taken by process " << rank << ": "
                  << duration.count() << " microseconds" << std::endl;
        std::cout << "Total sum of all elements: " << totalSum << std::endl;
    }

    MPI_Finalize();
}
