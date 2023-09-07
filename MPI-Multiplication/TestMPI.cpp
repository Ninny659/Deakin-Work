#include <stdlib.h>
#include <stdio.h>
#include "mpi.h"
#include <time.h>
#include <sys/time.h>

#define N 50

MPI_Status status;

int main(int argc, char **argv)
{
  int numProc, rank, workerTasks, taskAssignee, matrixRows, rowStartingPos;
  int M1[N][N], M2[N][N], M3[N][N];

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numProc);

  workerTasks = numProc - 1;

  if (rank == 0)
  {
    srand(time(NULL));
    for (int i = 0; i < N; i++)
    {
      for (int j = 0; j < N; j++)
      {
        M1[i][j] = rand() % 10;
        M2[i][j] = rand() % 10;
      }
    }

    matrixRows = N / workerTasks;
    rowStartingPos = 0;

    // Broadcast M2 to all workers
    MPI_Bcast(M2, N * N, MPI_INT, 0, MPI_COMM_WORLD);

    for (taskAssignee = 1; taskAssignee <= workerTasks; taskAssignee++)
    {
      MPI_Send(&rowStartingPos, 1, MPI_INT, taskAssignee, 1, MPI_COMM_WORLD);
      MPI_Send(&matrixRows, 1, MPI_INT, taskAssignee, 1, MPI_COMM_WORLD);
      MPI_Send(&M1[rowStartingPos][0], matrixRows * N, MPI_INT, taskAssignee, 1, MPI_COMM_WORLD);
      rowStartingPos = rowStartingPos + matrixRows;
    }

    for (int i = 1; i <= workerTasks; i++)
    {
      MPI_Recv(&rowStartingPos, 1, MPI_INT, i, 2, MPI_COMM_WORLD, &status);
      MPI_Recv(&matrixRows, 1, MPI_INT, i, 2, MPI_COMM_WORLD, &status);
      MPI_Recv(&M3[rowStartingPos][0], matrixRows * N, MPI_INT, i, 2, MPI_COMM_WORLD, &status);
    }
  }

  if (rank > 0)
  {
    MPI_Bcast(M2, N * N, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Recv(&rowStartingPos, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
    MPI_Recv(&matrixRows, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
    MPI_Recv(&M1[rowStartingPos][0], matrixRows * N, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);

    for (int i = 0; i < N; i++)
    {
      for (int j = 0; j < matrixRows; j++)
      {
        M3[j][i] = 0;
        for (int z = 0; z < N; z++)
        {
            M3[j][i] += M1[j][z] * M2[z][i];
        }
      }
    }

    MPI_Send(&rowStartingPos, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
    MPI_Send(&matrixRows, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
    MPI_Send(&M3[rowStartingPos][0], matrixRows * N, MPI_INT, 0, 2, MPI_COMM_WORLD);
  }

  MPI_Finalize();
}
