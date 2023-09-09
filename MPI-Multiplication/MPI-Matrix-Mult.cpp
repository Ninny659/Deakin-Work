#include <iostream>
#include <random>
#include <utility>
#include <functional>
#include <chrono>
#include <fstream>
#include <mpi.h>

using namespace std;

// Matrix size, cannot be increased due to memory limitations of VM
#define N 835

// MPI Status
MPI_Status status;

// Min and Max for array rands
const int minRnd = 5;
const int maxRnd = 12;

struct MatrixArgs
{
    int **m1, **m2, **m3;
};

// A function to generate and fill matrixs' with random values. If true then the resulting matrix will be filled with 0's.
int** matrixGenerator(bool resultMatrix)
{
    // Defining and allocating memory to the matrix to the size of N.
    int **matrix = (int **)malloc(N * sizeof(int *));

    for (int i = 0; i < N; i++)
    {
        matrix[i] = (int *)malloc(N * sizeof(int));
    }

    // Running through each position of the matrix and filling it
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            if (!resultMatrix)
            {
                matrix[i][j] = rand() % 10;
            }
            else
            {
                matrix[i][j] = 0;
            }
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

    std::chrono::milliseconds elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Calculate the time taken from the start of the clock to the end and return in milliseconds.
    return elapsed.count();
}

void *mpitication(void *args)
{
    struct MatrixArgs *mpi_task = (struct MatrixArgs *)args;

    // Defining the matrixs' to be used
    int M1[N][N], M2[N][N], M3[N][N];

    // Attempted to use the struct to pass the matrixs' but it was not working
    // int **M1, **M2, **M3;
    // M1 = mpi_task->m1;
    // M2 = mpi_task->m2;
    // M3 = mpi_task->m3;


    // Defining MPI variables
    int rank, numProc, workerTasks, taskAssignee, matrixRows, rowStartingPos = 0;

    // Initalising MPI ranks and number of processes
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numProc);

    // Defining the number of tasks to be done by each worker
    workerTasks = numProc - 1;

    if (rank == 0)
    {
        // Randomising the matrixs' values
        srand(time(NULL));
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < N; j++)
            {
                M1[i][j] = rand() % 10;
                M2[i][j] = rand() % 10;
            }
        }

        // Defining the number of rows to be calculated by each worker
        matrixRows = N / workerTasks;
        rowStartingPos = 0;

        // Broadcast M2 to all workers
        MPI_Bcast(M2, N * N, MPI_INT, 0, MPI_COMM_WORLD);

        // Send each worker its portion of the matrix to be calculated
        for (taskAssignee = 1; taskAssignee <= workerTasks; taskAssignee++)
        {
            // Send the starting position of the matrix to be calculated
            MPI_Send(&rowStartingPos, 1, MPI_INT, taskAssignee, 1, MPI_COMM_WORLD);
            MPI_Send(&matrixRows, 1, MPI_INT, taskAssignee, 1, MPI_COMM_WORLD);
            MPI_Send(&M1[rowStartingPos][0], matrixRows * N, MPI_INT, taskAssignee, 1, MPI_COMM_WORLD);

            // Increment the starting position of the matrix to be calculated
            rowStartingPos = rowStartingPos + matrixRows;
        }

        // Receive the calculated matrix from each worker
        for (int i = 1; i <= workerTasks; i++)
        {
            // Receive the starting position of the matrix to be calculated
            MPI_Recv(&rowStartingPos, 1, MPI_INT, i, 2, MPI_COMM_WORLD, &status);
            MPI_Recv(&matrixRows, 1, MPI_INT, i, 2, MPI_COMM_WORLD, &status);
            MPI_Recv(&M3[rowStartingPos][0], matrixRows * N, MPI_INT, i, 2, MPI_COMM_WORLD, &status);
        }
    }

    // If the rank is greater than 0 then it is a worker
    if (rank > 0)
    {
        // Receive M2 from the head
        MPI_Bcast(M2, N * N, MPI_INT, 0, MPI_COMM_WORLD);
        // Receive the starting position of the matrix to be calculated
        MPI_Recv(&rowStartingPos, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
        MPI_Recv(&matrixRows, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
        MPI_Recv(&M1[rowStartingPos][0], matrixRows * N, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);

        // Calculate the matrix
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < matrixRows; j++)
            {
                // Set the value to 0 to avoid garbage values
                M3[j][i] = 0;
                for (int z = 0; z < N; z++)
                {
                    // Calculate the matrix
                    M3[j][i] += M1[j][z] * M2[z][i];
                }
            }
        }

        // Send the calculated matrix back to the head
        MPI_Send(&rowStartingPos, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
        MPI_Send(&matrixRows, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
        MPI_Send(&M3[rowStartingPos][0], matrixRows * N, MPI_INT, 0, 2, MPI_COMM_WORLD);
    }

    return 0;
}

// Defining helper function to reduce code in the main() func
double MPIHelper()
{

    // Declaring a double timer result (Seconds) to print later
    double executionTime_blazingtication = timerFunction([&]()
                                                         {
        // Defining the struct to be passed by refrence, couldn't get it to work with pointers
        struct MatrixArgs mpi_task;
        mpi_task.m1 = matrixGenerator(false);
        mpi_task.m2 = matrixGenerator(false);
        mpi_task.m3 = matrixGenerator(true);

        // Assining all of our values to the struct to be passed by refrence
        mpitication((void *)&mpi_task); 
        });

    // Return the execution time
    return executionTime_blazingtication;
}

int main(int argc, char **argv)
{
    // Initialise MPI
    MPI_Init(&argc, &argv);

    int mpiTimed = MPIHelper();

    // Finalize MPI
    MPI_Finalize();

    // Print the execution time. Even though MPI has ended, the function prints two times for some reason.
    std::cout << "--------------------------------------------------------------\n"
                 "Execution Time for "
              << N << " elements (MPItication): " << mpiTimed << " milliseconds \n";

    // wait for key press
    cin.get();
    return 0;
}