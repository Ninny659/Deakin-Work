#include <iostream>

#include <mpi.h>

#include <cstring>

using namespace std;

// int main(int argc, char **argv)
// {
//     MPI_Init(&argc, &argv);
//     int numTasks, rank, tag = 1;
//     char charMSG[30] = "Hello World!";
//     char recvMSG[30];
//     char name[MPI_MAX_PROCESSOR_NAME];
//
//     MPI_Status status;
//     MPI_Comm_rank(MPI_COMM_WORLD, &rank);
//     MPI_Comm_size(MPI_COMM_WORLD, &numTasks);
//
//     if (rank == 0){ // Head process
//         // Using MPI_Send to send message to each worker
//         for (size_t i = 1; i < numTasks; i++)
//         {
//             //sending message to each worker
//             MPI_Send(&charMSG, std::strlen(charMSG), MPI_CHAR, i, tag, MPI_COMM_WORLD);
//         }
//     }
//     else{ // Node processes
//         // Nodes receive the message from the head process
//         MPI_Recv(&recvMSG, 30, MPI_CHAR, 0, tag, MPI_COMM_WORLD, &status);
//
//         // Print the received message
//         printf("Process rank %d : Message: %s\n", rank, recvMSG);
//     }
//
//     MPI_Finalize();
//     return 0;
// }

int main(int argc, char **argv)
{

    MPI_Init(&argc, &argv);

    int numTasks, rank = 1;

    char charMSG[30];

    char name[MPI_MAX_PROCESSOR_NAME];

    MPI_Status status;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Comm_size(MPI_COMM_WORLD, &numTasks);

    if (rank == 0)

    { // Master process

        // Printing message while inside the Head and assigning the message to be broadcast to charMSG

        printf("Hello in main MPI - HEAD, will broadcast message: \n");

        strcpy(charMSG, "Hello Processors");
    }

    // Broadcasting the message to all processing nodes

    MPI_Bcast(&charMSG, 30, MPI_CHAR, 0, MPI_COMM_WORLD);

    if (rank != 0)

    { // Worker processes

        printf("Process rank %d : Message: %s\n", rank, charMSG);
    }

    MPI_Finalize();

    return 0;
}
