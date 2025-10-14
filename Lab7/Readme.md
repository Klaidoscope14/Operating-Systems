# 🧮 Operating Systems Lab — Assignment 07

## 📘 Overview

This lab focuses on two classic Operating System problems related to **deadlocks** and **resource allocation safety**.

### 🧠 Problem 1 — Dining Philosophers with Deadlock Detection & Recovery

In this problem, we simulate five philosophers sitting around a table. Each philosopher alternates between **thinking** and **eating**. To eat, they must acquire both forks adjacent to them.

This implementation demonstrates:

* Deadlock-prone resource acquisition (left fork first, then right).
* Deadlock detection by the parent process.
* Recovery mechanism through random philosopher preemption.

#### 🧩 Key Features

* Uses **Pthreads** for concurrent philosopher processes.
* Uses **Semaphores** to synchronize fork access.
* **Parent thread** periodically detects and recovers from deadlocks.
* Logs all actions, including fork grabs, releases, and recovery events.

#### ⚙️ How It Works

1. Each philosopher repeatedly:

   * Thinks for a random duration.
   * Attempts to grab their left and right forks.
   * Eats for a random duration.
   * Releases both forks.
2. The parent process periodically checks for deadlock.
3. If all philosophers are hungry (deadlocked), the parent randomly selects one to preempt.

#### 🚀 To Run

```bash
gcc dining_philosophers.c -lpthread -o dining_philosophers
./dining_philosophers
```

Use `Ctrl + C` to stop the simulation manually.

#### 🧾 Sample Output

```
Philosopher 0 grabs fork 0 (left)
Philosopher 0 grabs fork 1 (right)
Philosopher 0 starts eating...
[Parent detects DEADLOCK! Initiating recovery...]
[Parent preempts Philosopher 2, forcing fork release]
```

---

### 🏦 Problem 2 — Banker’s Algorithm (Multithreaded)

Implements the **Banker’s Algorithm** to ensure safe resource allocation among concurrent processes.

#### 🧩 Key Features

* Multi-threaded using **Pthreads**.
* **Mutex locks** ensure safe access to shared data.
* Each thread (process) requests random resources within its need limits.
* The banker grants requests only if the system remains in a **safe state**.

#### ⚙️ How It Works

1. Initialize resource matrices: `Available`, `Max`, `Allocation`, and `Need`.
2. Each thread simulates a process making random requests.
3. Banker checks the safety condition before granting a request.
4. Unsafe requests are rolled back.

#### 🚀 To Run

```bash
gcc bankers_algorithm.c -lpthread -o bankers
./bankers
```

Program ends automatically after all processes complete.

#### 🧾 Sample Output

```
Process 1 making a request...
Request: 1 0 2
System is safe. Request granted for Process 1.
Simulation complete.
```

---

## 🧰 Tools & Dependencies

* Language: **C (GCC)**
* Libraries: `pthread.h`, `semaphore.h`, `unistd.h`, `stdlib.h`, `stdio.h`

---

## 🧑‍💻 Author

**Chaitanya Saagar**
Operating Systems Laboratory — Assignment 07
Department of Information Technology

---

## 🏁 Submission Notes

* Ensure both C files compile successfully.
* Include screenshots or logs of output for verification.
* Maintain proper indentation and comments.

---

### 🏷️ Files Included

| File                    | Description                                     |
| ----------------------- | ----------------------------------------------- |
| `dining_philosophers.c` | Simulation with deadlock detection and recovery |
| `bankers_algorithm.c`   | Multithreaded Banker’s Algorithm implementation |
| `README.md`             | This documentation file                         |