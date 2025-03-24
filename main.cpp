#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include <vector>
#include <queue>
#include <chrono>

// Dungeon instance
struct Instance {
    bool active;          // whether the instance currently has a running party
    int partiesServed;    // how many parties have used this instance
    int totalTimeServed;  // total sum of dungeon times (in seconds) for this instance

    Instance() : active(false), partiesServed(0), totalTimeServed(0) {}
};

std::mutex mtx;
std::condition_variable cv;

//   n      = number of concurrent instances
//   tTanks = number of tank players in the queue
//   tHeals = number of healer players in the queue
//   tDPS   = number of dps players in the queue
//   t1, t2 = min/max dungeon clear times
int n, tTanks, tHeals, tDPS, t1, t2;

std::vector<Instance> instances;
std::queue<int> freeInstances;
bool noMoreParties = false;

// Random generator setup
std::random_device rd;
std::mt19937 gen(rd());

// Utility function
void printInstances()
{
    std::cout << "\n--- Current Instance Status ---\n";
    for (int i = 0; i < n; i++) {
        if (instances[i].active) {
            std::cout << "Instance " << i << ": active\n";
        } else {
            std::cout << "Instance " << i << ": empty\n";
        }
    }
    std::cout << "--------------------------------\n";
}

// Simulates dungeon run
void runDungeon(int instanceID, int runTime)
{
    {
        std::lock_guard<std::mutex> lock(mtx);
        instances[instanceID].active = true;
        std::cout << "[+] Party assigned to Instance " << instanceID
                  << " for " << runTime << " seconds.\n";
        printInstances();
    }

    // Simulate dungeon clear time
    std::this_thread::sleep_for(std::chrono::seconds(runTime));

    {
        std::lock_guard<std::mutex> lock(mtx);
        instances[instanceID].active = false;
        instances[instanceID].partiesServed++;
        instances[instanceID].totalTimeServed += runTime;

        std::cout << "[-] Party finished on Instance " << instanceID
                  << ". Total parties served by this instance: "
                  << instances[instanceID].partiesServed << "\n";
        printInstances();

        // Mark instance as free
        freeInstances.push(instanceID);
    }
    // Notify main thread that an instance has been freed
    cv.notify_one();
}

int main()
{
    // Read user input
    std::cout << "Enter n (max concurrent instances): ";
    std::cin >> n;
    std::cout << "Enter number of tanks in queue: ";
    std::cin >> tTanks;
    std::cout << "Enter number of healers in queue: ";
    std::cin >> tHeals;
    std::cout << "Enter number of DPS in queue: ";
    std::cin >> tDPS;
    std::cout << "Enter fastest clear time t1 (seconds): ";
    std::cin >> t1;
    std::cout << "Enter slowest clear time t2 (seconds): ";
    std::cin >> t2;

    // Initialize the instances
    instances.resize(n);
    for (int i = 0; i < n; i++) {
        freeInstances.push(i);
    }

    std::uniform_int_distribution<> dist(t1, t2);
    std::vector<std::thread> dungeonThreads;

    // Continuously form parties
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);

        // Check if party can be formed
        bool canFormParty = (tTanks >= 1 && tHeals >= 1 && tDPS >= 3);

        // Stop when no more parties can be formed
        if (!canFormParty) {
            noMoreParties = true;
            break;
        }

        // If no free instances, wait until one is freed
        while (freeInstances.empty() && !noMoreParties) {
            cv.wait(lock);
            canFormParty = (tTanks >= 1 && tHeals >= 1 && tDPS >= 3);
            if (!canFormParty) {
                noMoreParties = true;
                break;
            }
        }

        if (noMoreParties) {
            break;
        }

        // We have a free instance and enough players to form a party
        int freeID = freeInstances.front();
        freeInstances.pop();

        // Form one party
        tTanks -= 1;
        tHeals -= 1;
        tDPS   -= 3;

        // Generate random clear time
        int dungeonTime = dist(gen);

        // Start a new thread for that dungeon run
        dungeonThreads.emplace_back(runDungeon, freeID, dungeonTime);
    }

    // Join dungeon threads and let them finish
    {
        std::unique_lock<std::mutex> lock(mtx);
    }

    for (auto &th : dungeonThreads) {
        if (th.joinable()) {
            th.join();
        }
    }

    // Print summary
    std::cout << "\n========== Final Summary ==========\n";
    int idx = 0;
    for (auto &inst : instances) {
        std::cout << "Instance " << idx++
                  << " served " << inst.partiesServed << " parties"
                  << " | Total time served: " << inst.totalTimeServed << "s\n";
    }
    std::cout << "===================================\n";

    return 0;
}
