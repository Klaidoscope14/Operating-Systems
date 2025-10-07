### 🧠 OS Lab – Assignment 6

#### *Readers–Writers Problem Variants with POSIX Threads & Semaphores*

---

#### 🎯 Objective

Implement and compare three synchronization strategies — Readers’ Preference, Writers’ Preference, and Fair FCFS (First Come First Served) — using **POSIX threads (`pthread`)** and **semaphores (`sem_t`)** in C.

---

### 📁 Files Overview

| File                      | Description                                   |
| ------------------------- | --------------------------------------------- |
| 🟢 `q1_readers_pref.c`    | Readers’ preference (finite accesses)         |
| 🔵 `q2_writers_pref.c`    | Writers’ preference (infinite loop)           |
| 🟣 `q3_fcfs_researcher.c` | FCFS fairness + Researchers (finite accesses) |

---

### 🧩 Problem Summaries

#### 🟢 Q1 — Readers’ Preference

* Multiple readers can read simultaneously.
* Writers gain exclusive access.
* Readers are not blocked unless a writer is active.
* Each thread performs a fixed number of accesses.

**Run:**

```bash
gcc -Wall -pthread q1_readers_pref.c -o q1_readers_pref
./q1_readers_pref 5 2 3
```

---

#### 🔵 Q2 — Writers’ Preference

* Writers have higher priority than readers.
* New readers cannot start reading while a writer is waiting.
* Infinite loop – terminate with **Ctrl + C**.

**Run:**

```bash
gcc -Wall -pthread q2_writers_pref.c -o q2_writers_pref
./q2_writers_pref 5 2
```

---

#### 🟣 Q3 — FCFS Fairness + Researchers

* Implements **First-Come, First-Served** fairness to prevent starvation.
* Allows batching of consecutive readers.
* Adds **Researchers**: behave like writers but have the **lowest priority**.

**Run:**

```bash
gcc -Wall -pthread q3_fcfs_researcher.c -o q3_fcfs_researcher
./q3_fcfs_researcher 3 2 1 3
```

---

### 🧵 Thread Roles

| Role                   | Behavior                            | Access Type            |
| ---------------------- | ----------------------------------- | ---------------------- |
| 👨‍🎓 Student (Reader) | Reads manuscript concurrently       | Shared read access     |
| 📚 Librarian (Writer)  | Updates manuscript exclusively      | Exclusive write access |
| 🔬 Researcher          | Special writer with lowest priority | Exclusive write access |

---

### 🔧 Synchronization Tools

| Primitive                                 | Purpose                        |
| ----------------------------------------- | ------------------------------ |
| `mutex`, `wrt`                            | Protect shared variables (Q1)  |
| `resource`, `rmutex`, `wmutex`, `readTry` | Writers’ priority control (Q2) |
| `queue_mutex`, `sem_t sem` per node       | Maintain FCFS order (Q3)       |

---

### 🧪 Example Outputs

#### Q1

```
Student 1 is waiting to read.
Student 1 is now reading (version: 0).
Librarian 1 is waiting to write.
Student 2 is now reading (version: 0).
```

#### Q2

```
[Librarian 1] wants to write.
[Student 1] wants to read.
[Librarian 1] is WRITING.
[Librarian 1] finished WRITING (new version 1).
```

#### Q3

```
[Student 1] START READING (version 0)
[Librarian 1] START WRITING
[Researcher 1] START WRITING (RESEARCH)
All threads completed. Final manuscript version: 9
```

---

### 💡 macOS Note

macOS marks `sem_init()` and `sem_destroy()` as deprecated. You can safely ignore these warnings or switch to named semaphores (`sem_open`, `sem_close`, `sem_unlink`).

---

### 👨‍💻 Author

**Chaitanya Saagar**
5th Semester — Operating Systems Laboratory
Department of Information Technology

---

### 🏁 Run Summary

| Program | Priority Model      | Duration | Stop      |
| ------- | ------------------- | -------- | --------- |
| Q1      | Readers’ Preference | Finite   | Auto exit |
| Q2      | Writers’ Preference | Infinite | Ctrl + C  |
| Q3      | FCFS + Researchers  | Finite   | Auto exit |

---

> 🧠 *This lab demonstrates synchronization, mutual exclusion, and starvation-free design using semaphores and pthreads — foundational concepts for modern operating systems.*