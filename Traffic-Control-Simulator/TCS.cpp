#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>

const int BUFFER_SIZE = 5;
const int NUM_ITEMS = 20;
const int NUM_SIGNALS = 4; 
const int NUM_MEASUREMENTS_PER_HOUR = 12;
const int MEASUREMENT_INTERVAL_MINUTES = 5;

struct TrafficData
{
    int timestamp, signalId, carsPassed;
};

std::queue<TrafficData> buffer;
std::mutex mutexLock;
std::condition_variable bufferNotEmpty;
std::condition_variable bufferNotFull;

void trafficProducer()
{
    for (int i = 0; i < NUM_ITEMS; ++i)
    {
        std::this_thread::sleep_for(std::chrono::seconds(MEASUREMENT_INTERVAL_MINUTES));

        TrafficData data;
        data.timestamp = i * MEASUREMENT_INTERVAL_MINUTES;
        data.signalId = i % NUM_SIGNALS;
        data.carsPassed = std::rand() % 100; 

        std::unique_lock<std::mutex> lock(mutexLock);
        bufferNotFull.wait(lock, []
                           { return buffer.size() < BUFFER_SIZE; });

        buffer.push(data);
        std::cout << "Produced: Timestamp = " << data.timestamp << ", Signal = " << data.signalId << ", Cars = " << data.carsPassed << std::endl;

        lock.unlock();
        bufferNotEmpty.notify_one();
    }
}

void trafficConsumer()
{
    std::vector<int> congestionCount(NUM_SIGNALS, 0);

    for (int i = 0; i < NUM_ITEMS; ++i)
    {
        std::unique_lock<std::mutex> lock(mutexLock);
        bufferNotEmpty.wait(lock, []
                            { return !buffer.empty(); });

        TrafficData data = buffer.front();
        buffer.pop();

        congestionCount[data.signalId] += data.carsPassed;

        lock.unlock();
        bufferNotFull.notify_one();
    }


    int N = 2;
    std::vector<std::pair<int, int>> signalCongestionPairs; 
    for (int i = 0; i < NUM_SIGNALS; ++i)
    {
        signalCongestionPairs.emplace_back(i, congestionCount[i]);
    }
    std::partial_sort(signalCongestionPairs.begin(), signalCongestionPairs.begin() + N, signalCongestionPairs.end(),
                      [](const std::pair<int, int> &a, const std::pair<int, int> &b)
                      {
                          return a.second > b.second;
                      });

    std::cout << "Top " << N << " congested traffic signals:" << std::endl;
    for (int i = 0; i < N; ++i)
    {
        std::cout << "Signal " << signalCongestionPairs[i].first << " - Congestion: " << signalCongestionPairs[i].second << " cars" << std::endl;
    }
}

int main()
{
    std::thread trafficProducerThread(trafficProducer);
    std::thread trafficConsumerThread(trafficConsumer);    trafficProducerThread.join();
    trafficConsumerThread.join();

    return 0;
}
