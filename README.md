# Dungeon Party Simulator

A simple command-line program that simulates players joining dungeon instances.

## Requirements

- A C++ compiler supporting at least C++11

## Building

### Using `g++` (Linux/Mac/MinGW)

1. Open your terminal (Linux/Mac) or MinGW shell (Windows).  
2. Navigate to the folder containing `main.cpp`.  
3. Compile with:
   ```
   g++ -std=c++11 -pthread main.cpp -o main
   ```
   - `-std=c++11` instructs the compiler to enable C++11 features.
   - `-pthread` links the pthread library (required for multithreading).

If you have an older compiler that doesn’t default to C++11, using `-std=gnu++11` instead of `-std=c++11` may help.

## Running

After compiling, run the resulting executable:

**On Linux/Mac**:
```
./main
```

**On Windows** (MinGW shell):
```
./main.exe
```

## Usage

The program will prompt you for the following inputs:

1. **n** (max concurrent instances)  
2. **tTanks** (number of tanks in the queue)  
3. **tHeals** (number of healers in the queue)  
4. **tDPS** (number of DPS players in the queue)  
5. **t1** (fastest clear time, in seconds)  
6. **t2** (slowest clear time, in seconds)

It will then attempt to form parties (1 tank, 1 healer, 3 DPS) until there are not enough players left or until no instances are free. Each party is assigned to a free instance, runs the dungeon in a random time between `t1` and `t2` seconds, then finishes, freeing the instance.

When you run out of valid party formations, the program displays:
- A summary of each instance (how many parties it served and total time served).
- The number of leftover tanks, healers, and DPS players.
