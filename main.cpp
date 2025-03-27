#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include <vector>
#include <queue>
#include <chrono>
#include <limits>

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
    if (!std::cin || n <= 0) {
        std::cerr << "Error: 'n' must be a positive integer.\n";
        return 1;
    }

    std::cout << "Enter number of tanks in queue: ";
    std::cin >> tTanks;
    if (!std::cin || tTanks < 0) {
        std::cerr << "Error: number of tanks cannot be negative.\n";
        return 1;
    }

    std::cout << "Enter number of healers in queue: ";
    std::cin >> tHeals;
    if (!std::cin || tHeals < 0) {
        std::cerr << "Error: number of healers cannot be negative.\n";
        return 1;
    }

    std::cout << "Enter number of DPS in queue: ";
    std::cin >> tDPS;
    if (!std::cin || tDPS < 0) {
        std::cerr << "Error: number of DPS players cannot be negative.\n";
        return 1;
    }

    std::cout << "Enter fastest clear time t1 (seconds): ";
    std::cin >> t1;
    if (!std::cin || t1 <= 0) {
        std::cerr << "Error: 't1' must be a positive integer.\n";
        return 1;
    }

    std::cout << "Enter slowest clear time t2 (seconds): ";
    std::cin >> t2;
    if (!std::cin || t2 <= 0) {
        std::cerr << "Error: 't2' must be a positive integer.\n";
        return 1;
    }

    if (t2 < t1) {
        std::cerr << "Error: t2 cannot be less than t1.\n";
        return 1;
    }

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
    std::cout << "\n============ Final Summary ============\n";
    int idx = 0;
    for (auto &inst : instances) {
        std::cout << "Instance " << idx++
                  << " served " << inst.partiesServed << " parties"
                  << " | Total time served: " << inst.totalTimeServed << "s\n";
    }
    std::cout << "=======================================\n";

    // Print leftover players
    std::cout << "\n========= Leftover Players =========\n";
    std::cout << "Tanks  : " << tTanks << "\n";
    std::cout << "Healers: " << tHeals << "\n";
    std::cout << "DPS    : " << tDPS   << "\n";
    std::cout << "====================================\n";

    return 0;
}
