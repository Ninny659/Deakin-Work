#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <fstream> 

using namespace std;

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

// Our traffic struct to store the traffic infomation in.
struct TrafficData
{
    int signalId, carsPassed;
    string timestamp;
};

// create traffic buffer to feed into our consumer
std::queue<TrafficData> buffer;

// Mutex to only allow threads to work sequentially.
std::mutex mutexLock;

// Buffer variables
std::condition_variable bufferNotEmpty;
std::condition_variable bufferNotFull;

void trafficProducer()
{
    // Check the date currently on the local machine, we will then assign to a char of size 11.
    auto currentTime = std::time(nullptr);
    auto localTime = *std::localtime(&currentTime);
    char timeStamp[11];

    // Declaring variables so we can increase the hour and minute with the MEASUREMENT_INTERVAL_MINUTES
    int hoursPassed = 10;
    int minutesPassed = 0;

    // Create a file stream to write data
    std::ofstream outputTrafficDataFile("traffic_data.txt");

    for (int i = 1; i <= NUM_MEASUREMENTS_PER_HOUR; ++i)
    {
        // Sleeping the thread for 5 minutes (Disabled for testing purposes)
        // std::this_thread::sleep_for(std::chrono::minutes(MEASUREMENT_INTERVAL_MINUTES));

        // For testing we are running 5 seconds instead of minutes
        std::this_thread::sleep_for(std::chrono::seconds(MEASUREMENT_INTERVAL_MINUTES));

        // Declaring our struct to be used later.
        TrafficData trafficData;

        // Checking if an hour has occured
        if (minutesPassed >= 60)
        {
            // if it has we iterate the hour and reset the minutes back to default.
            hoursPassed++;
            minutesPassed = 0;
        }

        // assign time stamp to a string the size of it's self
        std::strftime(timeStamp, sizeof(timeStamp), "%d-%m-%Y", &localTime);

        trafficData.timestamp = timeStamp;
        // Generating our traffic signal id.
        trafficData.signalId = i % NUM_SIGNALS;
        trafficData.carsPassed = std::rand() % 100;

        // Locking the code so threads have to run the code sequentially.
        std::unique_lock<std::mutex> lock(mutexLock);

        // Awaiting the consumer to consume the buffer.
        bufferNotFull.wait(lock, []
                           { return buffer.size() < BUFFER_SIZE; });

        // Pushing all of data into the buffer to be consumed by the consumer.
        buffer.push(trafficData);

        // Displaying the traffic signal.
        std::cout << "Produced: Timestamp = " << trafficData.timestamp << " - Hour: " << hoursPassed << " | Minutes: " << minutesPassed
                  << ", Signal = " << trafficData.signalId << ", Cars = " << trafficData.carsPassed << std::endl;

        // Write to file to produce an example input for the buffer. 
        outputTrafficDataFile << "Produced: Timestamp = " << trafficData.timestamp << " - Hour: " << hoursPassed << " | Minutes: " << minutesPassed
                   << ", Signal = " << trafficData.signalId << ", Cars = " << trafficData.carsPassed << std::endl;

        // Removing mutex lock.
        lock.unlock();
        bufferNotEmpty.notify_one();

        // Increasing the minutes passed.
        minutesPassed += MEASUREMENT_INTERVAL_MINUTES;
    }

    // Close the output file
    outputTrafficDataFile.close();
}

void trafficConsumer()
{
    // Created an array to assign the congested lane ids and cars passed
    std::vector<int> congestionCount(NUM_SIGNALS, 0);

    cout << "Consumer thread started" << endl;

    for (int i = 0; i < NUM_MEASUREMENTS_PER_HOUR; ++i)
    {
        std::unique_lock<std::mutex> lock(mutexLock);

        // Wait until the buffer is not empty, releasing the lock in the meantime
        bufferNotEmpty.wait(lock, []
                            { return !buffer.empty(); });

        // Retrieve data from the front of the buffer
        TrafficData data = buffer.front();

        // Remove the retrieved data from the front of the buffer
        buffer.pop();

        // Assigning the cars passed to the congested count.
        congestionCount[data.signalId] += data.carsPassed;

        lock.unlock();
        bufferNotFull.notify_one();
    }

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
}

int main()
{
    // Running our simulation
    std::thread trafficProducerThread(trafficProducer);
    std::thread trafficConsumerThread(trafficConsumer);

    // Once completed we return the threads back to the pool.
    trafficProducerThread.join();
    trafficConsumerThread.join();

    return 0;
}
