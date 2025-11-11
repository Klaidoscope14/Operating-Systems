# OS Lab Assignment 2 – README

## Introduction

This assignment explores the `/proc` filesystem and Linux system utilities to gather hardware, process, and system call information. The `/proc` filesystem is a virtual interface to kernel data, allowing real-time inspection of CPU, memory, disk, network, and process details.

The goal is to understand how the operating system reports and manages system resources, and how processes interact with the kernel via system calls.

---

## Question 1 – Basic System Information

### (a) CPU Sockets, Cores, and CPUs

```bash
grep -c '^processor' /proc/cpuinfo   # Logical CPUs
awk '/physical id/ {ids[$NF]=1} END{print length(ids)}' /proc/cpuinfo   # CPU sockets
lscpu | grep 'Core(s) per socket'     # Cores per socket
```

**Explanation:**

* `/proc/cpuinfo` lists processor details.
* Logical CPUs = number of `processor` entries.
* Physical sockets identified from unique `physical id` values.
* Cores per socket from `lscpu` output.

### (b) CPU Model Name and Stepping

```bash
awk -F: '/model name/ {print $2; exit}' /proc/cpuinfo
awk -F: '/stepping/ {print $2; exit}' /proc/cpuinfo
```

**Explanation:** Extracts `model name` and `stepping` fields from `/proc/cpuinfo`.

### (c) Frequency of Each CPU

```bash
grep 'cpu MHz' /proc/cpuinfo
```

**Explanation:** Lists the current operating frequency of each logical CPU.

### (d) Total Memory

```bash
grep MemTotal /proc/meminfo
```

**Explanation:** Shows total RAM in kilobytes.

### (e) Free vs Available Memory

```bash
grep -E 'MemFree|MemAvailable' /proc/meminfo
```

**Explanation:**

* `MemFree` = completely unused RAM.
* `MemAvailable` = memory available for new applications without swapping (includes caches/buffers).

### (f) Swap Memory Configured and Used

```bash
cat /proc/swaps
```

**Explanation:** Shows swap devices, sizes, and current usage.

### (g) Kernel Version

```bash
uname -r
```

**Explanation:** Displays the running Linux kernel version.

### (h) Number of User-Level Processes

```bash
ps -e --no-headers | wc -l
```

**Explanation:** Counts the number of processes currently running.

### (i) Total Context Switches Since Boot

```bash
awk '/^ctxt/ {print $2}' /proc/stat
```

**Explanation:** Reads the cumulative number of context switches from `/proc/stat`.

### (j) Uptime and Load Average

```bash
uptime
```

**Explanation:** Shows system uptime and load averages for the last 1, 5, and 15 minutes.

### (k) Size of Files in `/proc`

```bash
sudo du -sh /proc/*
```

**Explanation:** Reports the apparent sizes of entries in `/proc`. Most files show `0` because they are virtual.

### (l) Top 5 Memory-Consuming Processes

```bash
ps -axo pid,user,%mem,command | sort -k3 -nr | head -n 5
```

**Explanation:** Lists processes sorted by memory usage.

---

## Question 2 – Disk Statistics and Usage

### (a) Disk Read/Write Statistics

```bash
cat /proc/diskstats
```

**Explanation:** Displays I/O statistics per block device since boot.

### (b) Mounted Filesystem Usage

```bash
df -hT
```

**Explanation:** Shows disk usage, filesystem type, and mount points.

---

## Question 3 – Network Statistics

### (a) RX and TX Packets per Interface

```bash
cat /proc/net/dev
```

**Explanation:** Provides network interface statistics including packets received (RX) and transmitted (TX).

### (b) Current Network Configuration

```bash
ip addr show
```

**Explanation:** Displays IP addresses, interface states, and MAC addresses.

---

## Question 4 – File Descriptors

### (a) Number of Open File Descriptors for a Process

```bash
ls -l /proc/<PID>/fd | wc -l
```

**Explanation:** Counts open file descriptors in `/proc/<PID>/fd`.

### (b) List of Open File Descriptors

```bash
ls -l /proc/<PID>/fd
```

**Explanation:** Shows symbolic links from file descriptors to files/sockets.

---

## Question 5 – Filesystems and Block Devices

### (a) Mounted Filesystems and Types

```bash
mount | column -t
```

**Explanation:** Lists mounted filesystems with mount points and types.

### (b) Block Devices and Sizes

```bash
lsblk -o NAME,SIZE,FSTYPE,MOUNTPOINT
```

**Explanation:** Lists block devices with sizes, filesystem types, and mount points.

---

## Question 6 – Process Control

Steps:

1. Start a process:

```bash
yes > /dev/null &
```

2. Find its PID:

```bash
ps -e | grep yes
```

3. Stop the process:

```bash
kill -STOP <PID>
```

4. Resume the process:

```bash
kill -CONT <PID>
```

**Explanation:** Demonstrates sending `SIGSTOP` and `SIGCONT` signals to pause and resume execution.

---

## Question 7 – System Calls with `strace`

### Trace `ls` and Show First 10 Calls

```bash
strace -o ls_trace.txt -f ls
head -n 10 ls_trace.txt
```

### Trace `cat`

```bash
strace -o cat_trace.txt -f cat /etc/hostname
head -n 10 cat_trace.txt
```

### Trace `grep`

```bash
strace -o grep_trace.txt -f grep root /etc/passwd
head -n 10 grep_trace.txt
```

### Compare Unique System Calls between `cat` and `grep`

```bash
grep -oP '^[a-z_]+' cat_trace.txt | sort -u > cat_syscalls.txt
grep -oP '^[a-z_]+' grep_trace.txt | sort -u > grep_syscalls.txt
diff cat_syscalls.txt grep_syscalls.txt
```

**Explanation:**

* `strace` records system calls.
* The first 10 calls give insight into program startup.
* Comparing unique calls helps identify differences in program behavior.

---

## Conclusion

This lab provides practical exposure to system-level monitoring and control using `/proc` and Linux utilities. By running these commands, we observe how the kernel exposes live system data, how processes are managed, and how user programs interact with the OS through system calls.