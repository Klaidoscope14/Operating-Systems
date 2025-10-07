
---

````markdown
# 🧠 OS Lab – Assignment 6  
### Readers–Writers Problem Variants (with POSIX Threads & Semaphores)

---

## 📚 Overview
This lab implements three classic synchronization problems using **POSIX threads (`pthread`)** and **semaphores (`sem_t`)** in C.

Each question extends the previous one to demonstrate different priority and fairness strategies between *readers* (students) and *writers* (librarians), and introduces *researchers* in the final version.

---

## ⚙️ Files
| File | Description |
|------|--------------|
| `q1_readers_pref.c` | Readers-preference implementation (finite accesses) |
| `q2_writers_pref.c` | Writers-preference implementation (infinite loop) |
| `q3_fcfs_researcher.c` | FCFS (fair) implementation with researchers (finite accesses) |

---

## 🧩 Problem Descriptions

### 🟢 Q1 — Readers’ Preference
- Multiple readers can read simultaneously.
- Writers get exclusive access.
- Readers are not blocked unless a writer is already writing.
- Each thread performs a fixed number of accesses.

**Compile**
```bash
gcc -Wall -pthread q1_readers_pref.c -o q1_readers_pref
````

**Run**

```bash
./q1_readers_pref <#students> <#librarians> <#accesses>
```

**Example**

```bash
./q1_readers_pref 5 2 3
```

---

### 🔵 Q2 — Writers’ Preference

* Writers are given priority over readers.
* New readers cannot start reading if a writer is waiting.
* Infinite loop: threads continuously read/write until manually stopped.

**Compile**

```bash
gcc -Wall -pthread q2_writers_pref.c -o q2_writers_pref
```

**Run**

```bash
./q2_writers_pref <#students> <#librarians>
```

**Example**

```bash
./q2_writers_pref 5 2
```

> Stop execution with **Ctrl + C**.

---

### 🟣 Q3 — FCFS Fairness + Researchers

* Implements a **First-Come, First-Served (FCFS)** queue to ensure fairness.
* Consecutive readers arriving together can read concurrently.
* Adds **Researchers**, who behave like writers but have the *lowest priority* — they start only if no one else is waiting.

**Compile**

```bash
gcc -Wall -pthread q3_fcfs_researcher.c -o q3_fcfs_researcher
```

**Run**

```bash
./q3_fcfs_researcher <#students> <#librarians> <#researchers> <#accesses>
```

**Example**

```bash
./q3_fcfs_researcher 3 2 1 3
```

---

## 🧵 Thread Roles

| Role                   | Behavior                                          | Shared Resource Access |
| ---------------------- | ------------------------------------------------- | ---------------------- |
| **Student (Reader)**   | Reads manuscript concurrently with other students | Shared read access     |
| **Librarian (Writer)** | Updates manuscript exclusively                    | Exclusive write access |
| **Researcher**         | Special writer with lowest priority               | Exclusive write access |

---

## 🧱 Synchronization Primitives

| Semaphore / Mutex                         | Purpose                                           |
| ----------------------------------------- | ------------------------------------------------- |
| `mutex`, `wrt`                            | Basic mutual exclusion for readers & writers (Q1) |
| `resource`, `readTry`, `wmutex`, `rmutex` | Enforce writers' preference (Q2)                  |
| `queue_mutex`, `sem_t sem` per thread     | Maintain FCFS order and batching (Q3)             |

---

## 🧪 Example Outputs

### Q1

```
Student 1 is waiting to read.
Student 1 is now reading (version: 0).
Librarian 1 is waiting to write.
Student 2 is now reading (version: 0).
Student 1 has finished reading.
...
```

### Q2

```
[Librarian 1] wants to write.
[Student 1] wants to read.
[Librarian 1] is WRITING.
[Librarian 1] finished WRITING (new version 1).
```

### Q3

```
[Student 1] START READING (version 0)
[Librarian 1] START WRITING
[Researcher 1] START WRITING (RESEARCH)
All threads completed. Final manuscript version: 9
```

---

## 🧰 Notes for macOS Users

macOS marks `sem_init()` and `sem_destroy()` as deprecated.
You can safely **ignore these warnings**, or replace with **named semaphores** (`sem_open`, `sem_close`, `sem_unlink`) if desired.

---

## 🧑‍💻 Author

**Chaitanya Saagar**
5th Semester — Operating Systems Laboratory
Department of Information Technology

---

## 🏁 Run Summary

| Program | Priority           | Duration | Stop      |
| ------- | ------------------ | -------- | --------- |
| Q1      | Readers-preference | Finite   | Auto exit |
| Q2      | Writers-preference | Infinite | Ctrl + C  |
| Q3      | FCFS + Researchers | Finite   | Auto exit |

---

> 🧠 *This lab demonstrates synchronization, mutual exclusion, and starvation-free design using semaphores and pthreads — foundational concepts for modern operating systems.*

```

---

Would you like me to make this a **styled Canva-formatted README** (like your AadhaarConnect one earlier) — single-page visual layout with sections and icons? I can output that version next.
```