#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>

using namespace std::chrono;
using namespace std;

void randomVector(int vector[], int size)
{
#pragma omp parallel default(none)
    #pragma omp for
    for (int i = 0; i < size; i++)
    {
        // Adding rand value to the vector.
        vector[i] = rand() % 100;
    }
}

int main()
{

    unsigned long size = 100000000;

    srand(time(0));

    int *v1, *v2, *v3;

    // Starting our timer and setting it to the timer_start variable.
    auto start = high_resolution_clock::now();

    // Setting up a vector of vectorSize - int vectorSize
    v1 = (int *)malloc(size * sizeof(int *));
    v2 = (int *)malloc(size * sizeof(int *));
    v3 = (int *)malloc(size * sizeof(int *));

    randomVector(v1, size);
    randomVector(v2, size);

    // Adding vector1 to vector2 to the result of vector3
#pragma omp parallel default(none) shared(size, v3) private(v1,v2)
    #pragma omp for schedule(static, chunk)
    for (int i = 0; i < size; i++)
    {
        #pragma omp atomic
        v3[i] = v1[i] + v2[i];
    }

#pragma omp critical
{
    // Stopping the timer to measure affectivness of our implmentation.
    auto timer_stop = high_resolution_clock::now();

    // Retriving the time value of stop - timer_start together, for result duration.
    auto duration = duration_cast<microseconds>(timer_stop - start);

    cout << "Time taken by function: "
         << duration.count() << " microseconds" << endl;

    return 0;
}
}