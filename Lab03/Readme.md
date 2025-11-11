# OS Lab — Process & Scheduling Mini-Suite (macOS/Linux)

This document explains four small C programs demonstrating **UNIX process management** and a user-space **scheduler simulation**.

> **Platform:** All sources compile on both **macOS** and **Linux** with `clang`/`gcc`.
> **Important macOS difference:** macOS does **not** provide `/proc`. Where the assignment mentions reading `/proc/<pid>/status`, use `ps`, `top`, and `vmmap` instead (details below).

---

## Contents

```
Lab3/
 ├─ Q1/ Q1.c   # Print PID, keep process alive; inspect from another terminal
 ├─ Q2/ Q2.c   # fork() demo: stdout buffer duplication + 2 children + waitpid()
 ├─ Q3/ Q3.c   # Process-state simulation (NEW→READY→RUNNING→BLOCKED→READY→FINISHED) with FIFO
 └─ Q4/ Q4.c   # Parent "mini-scheduler" controlling 3 children via SIGSTOP/SIGCONT
```

You can keep the files in one folder and build them individually; the directory layout above is just a suggestion.

---

## Prerequisites

* **Compiler:** `clang` (macOS default) or `gcc`
* **Terminal:** run programs from Terminal/iTerm (Q4 needs interactive stdin)
* **VS Code (optional):**

  * C/C++ extension configured to use **Clang**
  * **Compiler Path:** `/usr/bin/clang`
  * **IntelliSense Mode:** `macos-clang-arm64` (Apple Silicon) or `macos-clang-x64` (Intel)
  * **C Standard:** `c11`

---

## Build & Run (quick start)

From the folder containing the source file:

```bash
# macOS or Linux
clang -std=c11 -Wall -Wextra -O2 Q1.c -o q1
./q1
```

Repeat for `Q2.c`, `Q3.c`, and `Q4.c` (changing names accordingly).

### Optional: simple Makefile

```make
CC=clang
CFLAGS=-std=c11 -Wall -Wextra -O2

all: q1 q2 q3 q4

q1: Q1.c
	$(CC) $(CFLAGS) $< -o $@

q2: Q2.c
	$(CC) $(CFLAGS) $< -o $@

q3: Q3.c
	$(CC) $(CFLAGS) $< -o $@

q4: Q4.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f q1 q2 q3 q4
```

Build everything with `make`, clean with `make clean`.

---

## Q1 — Show PID and Inspect from Another Terminal

**Goal:** Start a process, print its PID, keep it alive for a while so you can inspect it.

### How to run

```bash
clang -std=c11 -Wall -Wextra -O2 Q1.c -o q1
./q1
# Example output
# PID: 12345
# Sleeping for 60 seconds. Inspect me from another terminal...
```

### How to inspect

* **Linux (as in many handouts):**

  ```bash
  cat /proc/<PID>/status
  ```

* **macOS (no /proc):** use the following equivalents:

  ```bash
  ps -o pid,ppid,stat,comm,thcount,vsz,rss -p <PID>
  top -pid <PID>         # live view; press q to quit
  vmmap <PID> | head     # memory region summary
  ```

**Field mapping (rough):**

* **Name** → `comm` (from `ps`)
* **State** → `stat` letter(s) (`R` running, `S` sleeping, etc.)
* **Pid/PPid** → `pid` / `ppid`
* **Threads** → `thcount`
* **VmSize** → approximated by `VSZ`/`RSS` (`ps`) or use `vmmap` for detail

**What cannot be done on macOS in Q1:** Read `/proc/<pid>/status` (there is no `/proc`). Everything else works.

---

## Q2 — `fork()` and stdout Buffer Duplication + Two Children + `waitpid()`

**Concepts demonstrated:**

* **Buffered I/O duplication:** A `printf` *without newline* before `fork()` lives in the parent’s stdout buffer and gets **duplicated** into each child’s buffer.
* **Process tree:** Parent forks **Child 1**, then forks **Child 2**.
* **Synchronization:** Parent waits for both children with `waitpid()`.

### Run

```bash
clang -std=c11 -Wall -Wextra -O2 Q2.c -o q2
./q2
```

### Expected behavior

* You’ll see `PREFORK ` echoed by all three processes (parent + 2 children) before their first line.
* Each process prints 3 timed lines with its role and (PID, PPID).
* Parent prints: `Both children finished execution`.

**Tip:** If you change `printf("PREFORK ");` to include a newline (`\n`), the duplication disappears because the buffer is flushed before `fork()`.

---

## Q3 — Process-State Simulation (FIFO, non-preemptive)

A **user-space simulation** of a simple scheduler with states:

`NEW → READY → RUNNING → BLOCKED → READY → FINISHED`

### Data structures

* `READY` queue (FIFO)
* `BLOCKED` list
* `running` pointer
* Each `Process` tracks `pid`, `cpu_total`, and `blocked_event`

### Core operations

* `newProcess()` — create process and enqueue to READY
* `cpuEvent()` — dispatch from READY if needed; give 1 “CPU tick” to RUNNING
* `blockEvent(eventId)` — move RUNNING to BLOCKED on given event
* `unblockEvent(eventId)` — move one matching BLOCKED process back to READY
* `doneEvent()` — terminate RUNNING process
* `printState()` — snapshot of RUNNING, READY, BLOCKED

### Run

```bash
clang -std=c11 -Wall -Wextra -O2 Q3.c -o q3
./q3
```

The provided `main()` scripts a plausible sequence. You can change it to exercise different paths (multiple `newProcess()`, interleaved blocks/unblocks, etc.).

---

## Q4 — Mini “Scheduler” using `SIGSTOP`/`SIGCONT` (Interactive)

The parent process forks 3 children and **controls** them like a scheduler:

* **P1:** prints even numbers `0..16`
* **P2:** prints odd numbers `1..15`
* **P3:** prints characters `A..J`

Each child **self-stops** right after `fork()` via `raise(SIGSTOP)`. The parent keeps a READY queue of child PIDs; for each scheduled process it sends **`SIGCONT`** to run. While the child runs, you can **press Enter** to block (parent sends `SIGSTOP`) and the child is **re-enqueued**. If you don’t press Enter, the child finishes and the parent moves on.

### Run

```bash
clang -std=c11 -Wall -Wextra -O2 Q4.c -o q4
./q4
# Follow the on-screen prompt:
# "Press Enter to BLOCK (requeue), or wait to let it run."
```

**Notes**

* Works the same on macOS and Linux.
* Use a real terminal (not a background console) so stdin is interactive.
* The parent uses `select()` to poll stdin once per second while simultaneously checking child status with `waitpid(..., WNOHANG)`.

---

## Troubleshooting & Common Errors

### `pid_t is undefined`

* Include `<sys/types.h>` and `<unistd.h>`.
* Example:

  ```c
  #include <sys/types.h>  // pid_t
  #include <unistd.h>     // getpid
  ```

### `identifier '__stdoutp' is undefined` (macOS)

* Include `<stdio.h>` (defines `stdout`, which maps to `__stdoutp` internally on macOS).
* Ensure the file is parsed as **C**, not C++.

### VS Code shows C++-style IntelliSense errors (e.g., “function call not allowed in a constant expression”)

* Your `.c` file is being parsed as C++. Fix:

  * File extension is `.c`
  * **Compiler Path:** `/usr/bin/clang`
  * **IntelliSense Mode:** `macos-clang-arm64` or `macos-clang-x64`
  * **C Standard:** `c11`

### Build succeeds but no output appears (Q4)

* Make sure you’re running in a terminal and not suppressing stdout.
* For tidier timing, we set line-buffering with `setvbuf(stdout, NULL, _IOLBF, 0);` in `Q4.c`.

### Killing stuck processes

* In another terminal: `ps aux | grep q[1-4]`
* Terminate by PID: `kill <pid>` (or `kill -9 <pid>` as last resort).

---

## macOS vs Linux Summary — What **can’t** be done on macOS?

* Reading `/proc/<pid>/status` (and other `/proc/*`) — **not available** on macOS. Use `ps`, `top -pid`, and `vmmap` instead.
* All other parts (fork/exec, waitpid, signals, select, etc.) work as used in this assignment.

---

## Academic Honesty

Use this content as a learning aid. Make sure you understand the concepts (process creation, buffering, waiting, signals, and simple scheduling) and can explain them. If your course has a policy on collaboration, follow it.

---

## Credits

Prepared for a UNIX OS lab covering: processes, buffering semantics, process state models, and user-space scheduling via signals.