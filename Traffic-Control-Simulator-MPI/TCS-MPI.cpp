#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <mpi.h>
#include <queue>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
#include <unistd.h>

// Our buffer size for the buffer
const int BUFFER_SIZE = 5;

// Change this to simulate x amount of hours
const int HOURS_RUN = 2;

// How many cycles to run, please change HOURS_RUN.
const int NUM_MEASUREMENTS_PER_HOUR = HOURS_RUN * 12;

// Amount of traffic signals to simulate
const int NUM_SIGNALS = 4;

// Measures every 5 minutes (For testing we are using seconds instead)
const int MEASUREMENT_INTERVAL_MINUTES = 5;

// Print the most congested signals. (HAS TO BE LESS THEN NUM_SIGNALS)
const int CONGESTED_SIGNAL_COUNT = 2;

// Our tag for the MPI_Send and MPI_Recv
const int TAG = 0;

// Starting hour
const int STARTING_HOUR = 10;

// Our traffic struct to store the traffic infomation in.
struct TrafficData
{
    int signalId, carsPassed;
    std::string timestamp;
};

void sendTrafficData(const TrafficData &data, int dest)
{
    // Send the 3 seperate data to the consumer
    MPI_Send(&data.signalId, 1, MPI_INT, dest, TAG, MPI_COMM_WORLD);
    MPI_Send(&data.carsPassed, 1, MPI_INT, dest, TAG, MPI_COMM_WORLD);
    const char *timestamp = data.timestamp.c_str();
    MPI_Send(timestamp, strlen(timestamp) + 1, MPI_CHAR, dest, TAG, MPI_COMM_WORLD);
}

void receiveTrafficData(TrafficData &data, int source)
{
    // Recieve the 3 seperate data from the producer
    MPI_Recv(&data.signalId, 1, MPI_INT, source, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&data.carsPassed, 1, MPI_INT, source, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    char timestamp[100];
    MPI_Recv(timestamp, sizeof(timestamp), MPI_CHAR, source, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    data.timestamp = timestamp;
}

int main(int argc, char *argv[])
{
    // Initialise the MPI environment
    MPI_Init(&argc, &argv);
    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    std::queue<TrafficData> buffer;

    // Check if we have 2 processes
    if (world_size != 2)
    {
        std::cerr << "This program requires exactly 2 processes: 1 producer and 1 consumer." << std::endl;

        MPI_Finalize();
        return 1;
    }

    // Check the date currently on the local machine, we will then assign to a char of size 11.
    auto currentTime = std::time(nullptr);
    auto localTime = *std::localtime(&currentTime);
    char timeStamp[11];
    int hoursPassed = STARTING_HOUR;
    int minutesPassed = 0;

    
    std::vector<int> congestionCount(NUM_SIGNALS, 0);

    for (int i = 1; i <= NUM_MEASUREMENTS_PER_HOUR; i++)
    {
        // Traffic producer
        if (world_rank == 0)
        {
            srand(time(nullptr));

            std::strftime(timeStamp, sizeof(timeStamp), "%d-%m-%Y", &localTime);

            // Generate traffic data
            TrafficData trafficData;
            trafficData.timestamp = timeStamp;
            trafficData.signalId = i % NUM_SIGNALS;
            trafficData.carsPassed = rand() % 100;
            buffer.push(trafficData);

            // Print the traffic data
            std::cout << "Produced: Timestamp = " << trafficData.timestamp << " - Hour: " << hoursPassed << " | Minutes: " << minutesPassed
                      << ", Signal = " << trafficData.signalId << ", Cars = " << trafficData.carsPassed << std::endl;

            // Send the traffic data to the consumer
            sendTrafficData(trafficData, 1);

            // Remove the traffic data from the buffer
            buffer.pop();
        }
        // Traffic consumer
        else
        {
            TrafficData trafficData;
            receiveTrafficData(trafficData, 0);

            // Update the congestion count for the respective signal
            congestionCount[trafficData.signalId] += trafficData.carsPassed;
        }
    
        if (minutesPassed >= 55 && world_rank == 1)
        {
            // Declare a vector to store signal-congestion pairs
            std::vector<std::pair<int, int>> signalCongestionPairs;

            // Populate the vector with signal indices and their corresponding congestion counts
            for (int i = 0; i < NUM_SIGNALS; ++i)
            {
                signalCongestionPairs.emplace_back(i, congestionCount[i]);
            }

            // Partially sort the vector to get top congested signals based on congestion counts
            std::partial_sort(signalCongestionPairs.begin(), signalCongestionPairs.begin() + CONGESTED_SIGNAL_COUNT, signalCongestionPairs.end(),
                              [](const std::pair<int, int> &a, const std::pair<int, int> &b)
                              {
                                  return a.second > b.second;
                              });

            // Print the top congested traffic signals and their congestion counts
            std::cout << "Top " << CONGESTED_SIGNAL_COUNT << " congested traffic signals:" << std::endl;
            for (int i = 0; i < CONGESTED_SIGNAL_COUNT; ++i)
            {
                std::cout << "Signal " << signalCongestionPairs[i].first << " - Congestion: " << signalCongestionPairs[i].second << " cars" << std::endl;
            }

            // Reset the congestion count for all signals
            std::fill(congestionCount.begin(), congestionCount.end(), 0);
        }

        MPI_Barrier(MPI_COMM_WORLD);
        sleep(MEASUREMENT_INTERVAL_MINUTES);
        // sleep(1);

        minutesPassed += MEASUREMENT_INTERVAL_MINUTES;
        if (minutesPassed >= 60)
        {
            hoursPassed++;
            minutesPassed = 0;
        }
    }

    MPI_Finalize();
    return 0;
}