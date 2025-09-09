# OS Lab Assignment — README

This repository contains solutions to the OS Lab Assignment (5th Set of Questions). The lab consists of three major parts:

---

## 📌 Part 1: Multi-Stage Pipeline

* **Description:**

  * Two producer threads generate random integers (0–99).
  * Two processor threads square the produced values.
  * One consumer thread stores results to a log file.
  * Uses **two bounded buffers** of size 10 each.
  * Ensures **no busy waiting**.
  * Processes exactly **N = 50 items**.

* **Implementation details:**

  * Producers place values into buffer1.
  * Processors take from buffer1, square, and push into buffer2.
  * Consumer takes from buffer2, writes to `log.txt`, and prints status.
  * Synchronization with **mutexes** and **condition variables**.
  * Sentinels (`-1`) used to terminate processors after producers finish.

* **Run:**

  ```bash
  gcc -O2 -pthread q1_pipeline.c -o q1
  ./q1
  ```

  Output is interleaved producer/processor/consumer logs. Results stored in `log.txt`.

---

## 📌 Part 2: Matrix Multiplication (One Thread per Cell)

* **Description:**

  * Compute C = A × B.
  * A is M×K, B is K×N, result C is M×N.
  * Spawns **M×N threads**, each computing exactly one element C\[i]\[j].
  * Matrices are **global**.

* **Implementation details:**

  * Struct `{i, j}` used to pass indices to each thread.
  * No extra synchronization needed (each thread writes to a unique cell).

* **Run:**

  ```bash
  gcc -O2 -pthread q2_matmul_threads.c -o q2
  ./q2
  ```

  Program prompts for matrix sizes and values.

---

## 📌 Part 3: The Royal Banquet — Philosophers’ Dilemma

* **Description:**

  * Variation of Dining Philosophers problem.
  * N philosophers, F forks (F < N).
  * Each philosopher needs 2 forks to eat.
  * **Dynamic joins/leaves:**

    * Philosopher 0 joins late (e.g., 5s).
    * Philosopher 3 leaves early (e.g., 30s).
  * **Fairness:** If a philosopher waits ≥ T seconds, they are prioritized.
  * Controlled by a **waiter thread** that grants forks fairly.
  * Banquet runs for **60 seconds**.

* **Implementation details:**

  * Shared fork pool (`forks_available`).
  * Queue for waiting philosophers.
  * Waiter enforces fairness to prevent starvation.
  * Collects statistics per philosopher:

    * Number of times eaten
    * Average wait time
    * Maximum wait time

* **Run:**

  ```bash
  gcc -O2 -pthread q3_royal_banquet.c -o q3
  ./q3
  ```

  Default: `N=6, F=4, T=1.0s, duration=60s`.

  You can also customize:

  ```bash
  ./q3 <N> <F> <T_seconds> <duration_seconds>
  ```

* **Example output:**

  ```
  Philosopher 0 arrives late to the banquet!
  Philosopher 3 leaves the banquet early!
  Philosopher 5 is eating...
  ...
  ===== Royal Banquet Report =====
  Philosopher 0: Ate 2 times | Avg Wait: 0.80s | Max Wait: 1.20s
  Philosopher 1: Ate 5 times | Avg Wait: 0.50s | Max Wait: 0.90s
  ...
  ```

---

## 📌 Makefile (for convenience)

To compile all parts at once:

```makefile
CC=gcc
CFLAGS=-O2 -pthread

all: q1 q2 q3

q1: q1_pipeline.c
	$(CC) $(CFLAGS) $< -o $@

q2: q2_matmul_threads.c
	$(CC) $(CFLAGS) $< -o $@

q3: q3_royal_banquet.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f q1 q2 q3 log.txt
```

Compile and run:

```bash
make
./q1
./q2
./q3
```

---

## ✅ Deliverables

* `q1_pipeline.c` — Multi-stage pipeline implementation
* `q2_matmul_threads.c` — One-thread-per-cell matrix multiplication
* `q3_royal_banquet.c` — Royal Banquet philosophers with fairness
* `Makefile`
* `README.md`

---

## 📚 Notes

* All programs are written in **C with POSIX Threads**.
* Make sure to compile with `-pthread`.
* Tested on Linux (gcc, Ubuntu).
* Logs and reports are printed to console (and `log.txt` for Part 1).