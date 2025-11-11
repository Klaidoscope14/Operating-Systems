# Virtual Memory and Fragmentation Simulation

## Objective

The purpose of this lab is to simulate different aspects of **memory management** in operating systems, focusing on:

1. Demand-paged virtual memory system (process interaction and page faults)
2. Page replacement algorithms (FIFO and LRU)
3. Variable partition allocation and external fragmentation

These programs demonstrate how the operating system handles memory allocation, page faults, replacement policies, and fragmentation analysis.

---

## Part A – Demand-Paged Virtual Memory Simulation

### Task

Implement a simulation that models the working of a demand-paged virtual memory system using four modules:

* **Master Process:** Initializes the system, creates other threads, and terminates them at the end.
* **Scheduler:** Implements FCFS scheduling for process execution.
* **Memory Management Unit (MMU):** Handles address translation, page faults, and replacement using LRU.
* **Process Threads:** Each process generates a reference string of pages and requests memory access.

### Implementation

* Implemented using **Python multithreading** to simulate concurrent processes.
* Processes randomly generate page reference strings.
* The MMU keeps page tables for each process and maps pages to frames.
* On a page fault, the MMU loads the page into memory using **LRU (Least Recently Used)** replacement policy.
* The Scheduler ensures processes are handled in FCFS order.

### Execution

```bash
python3 vm_sim.py
```

### Output

* Displays which process requests which page.
* Indicates page hits and page faults.
* Shows frame replacement actions and system termination message.

---

## Part B – Page Replacement Algorithms

### Task

Simulate the **FIFO** and **LRU** page replacement algorithms for a random page reference string and frame sizes from 1 to 7.

### Implementation

* Implemented in **C** for performance and clarity.
* Randomly generated reference string (pages 0–9).
* Simulates page replacement for frame sizes 1–7.
* FIFO uses circular frame replacement.
* LRU tracks timestamps to replace the least recently used page.

### Execution

```bash
gcc page_replace.c -o page_replace
./page_replace
```

### Output

* Prints the page reference string.
* Displays the number of page faults for both FIFO and LRU for each frame size.

---

## Part C – Variable Partition and Fragmentation Simulation

### Task

Simulate a **variable-partition memory allocation system** with random allocation and deallocation requests. Calculate and display fragmentation statistics.

### Implementation

* Implemented in **C**.
* Memory represented as linked list of blocks.
* Uses **First Fit** allocation strategy.
* Deallocation merges adjacent free partitions.
* After all operations, calculates:

  * Total free space
  * Largest contiguous free block
  * External fragmentation
  * Fragmentation ratio

### Execution

```bash
gcc fragmentation.c -o fragmentation
./fragmentation
```

### Sample Run

```
Enter total memory size (KB): 1000
Enter number of operations: 10
```

The program prints a step-by-step allocation/deallocation trace and final fragmentation statistics.

---

## Key Learnings

* Understood **virtual memory concepts** including page tables, page faults, and replacement policies.
* Implemented **FIFO and LRU** to compare their efficiency in minimizing page faults.
* Explored **memory fragmentation** and the challenges in dynamic allocation systems.
* Learned how **operating systems manage limited physical memory** efficiently.

---

## File Structure

```
OS_LAB_10/
├── vm_sim.py              # Demand-paged VM simulation
├── page_replace.c         # FIFO & LRU simulation
├── fragmentation.c        # Variable partition & fragmentation
└── README.md              # Documentation
```

---

## How to Run All Programs

1. Open terminal in the directory.
2. Run the Python script for Part A.
3. Compile and execute each C file for Part B and C.
4. Observe and record the results.

These programs collectively demonstrate the complete working of **virtual memory, page replacement, and fragmentation management** in an operating system.