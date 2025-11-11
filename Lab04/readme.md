# OS Lab Assignment – Interprocess Communication & Synchronization

This repository contains solutions to the OS assignment questions on **pipes**, **fork()**, **shared memory**, and **multi-process/multi-threaded coordination**.
All implementations are written in **C (C99)** and tested on **macOS (Clang)**.

---

## 📌 Setup (macOS)

First install developer tools:

```bash
xcode-select --install
```

Then clone/unpack the assignment folder and build:

```bash
cd os-lab
make
```

---

## 🔹 Q1 – Ordinary Pipes

**File:** `pipes_case.c`

* Uses **two pipes** for full duplex communication:

  * Parent → Child (input string)
  * Child → Parent (processed string)
* Child toggles **alphabet case** (upper ↔ lower).

**Run:**

```bash
./pipes_case "Hello OS Lab"
```

**Output Example:**

```
hELLO os lAB
```

---

## 🔹 Q2A – Fibonacci with fork()

**File:** `fib_fork.c`

* Parent forks a child.
* **Child** computes and prints `n` Fibonacci numbers.
* **Parent** waits until child finishes.

**Run:**

```bash
./fib_fork 7
```

**Output:**

```
0 1 1 2 3 5 8
```

---

## 🔹 Q2B – Fibonacci with POSIX Shared Memory

**File:** `fib_shm.c`

* Demonstrates **Inter-Process Communication (IPC)**.
* Parent creates a **POSIX shared memory segment**.
* **Child** computes Fibonacci and writes to memory.
* **Parent** reads the sequence and prints.

**Run:**

```bash
./fib_shm 10
```

**Output:**

```
0 1 1 2 3 5 8 13 21 34
```

⚠️ On macOS, you do **not** need `-lrt`. `clang` with `-pthread` is enough.

---

## 🔹 Q3 – Newsroom Simulation

**Files:**

* `editor.c` (spawns N journalists, collects M articles using `select()`).
* `journalist.c` (each process has 3 threads: researcher, writer, submitter, using mutex + condition variables).

**Run:**

```bash
./editor 3 10
```

**Output Example:**

```
Editor: Newsroom open. Waiting for 10 articles from 3 journalists.
Journalist 1, Researcher: Found topic "Story 1 from Journalist 1"
Journalist 1, Writer: Writing article on "Story 1 from Journalist 1"
Journalist 1, Submitter: Submitting article.
Editor: Published article! [1/10] -> "Article on Story 1 from Journalist 1"
...
Editor: Deadline met! Published 10 articles. Newsroom closed.
```

---

## 🧹 Clean Up

To remove all executables:

```bash
make clean
```

---

## ✅ Summary

* **Q1**: Pipes for bidirectional parent-child communication.
* **Q2A**: Fibonacci via `fork()`.
* **Q2B**: Fibonacci via **shared memory** IPC.
* **Q3**: Multi-process newsroom with **multi-threaded journalists** coordinated via pipes and synchronization primitives.