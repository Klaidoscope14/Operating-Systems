# CS3101 Operating Systems Lab - Assignment 9

## Title: Custom Memory Allocators Using `mmap()`

---

## 📘 Objective

The goal of this lab is to implement **two custom memory allocators** using the `mmap()` system call:

1. **Simple Allocator (`alloc`)** – works with a single 4KB memory page.
2. **Elastic Allocator (`ealloc`)** – dynamically expands by allocating additional pages on demand.

These allocators demonstrate how memory management can be done at the user level without relying on the system heap.

---

## 🧩 Task 1: Simple Allocator (`alloc`)

### **Goal:**

Implement a basic allocator that uses exactly one 4KB page of memory. The allocator must:

* Use `mmap()` to request one 4KB page.
* Keep **metadata** (like free lists) in normal heap memory, not inside the page.
* Support `alloc_mem(size)` and `dealloc_mem(ptr)`.
* Enforce allocations in multiples of **8 bytes**.
* Free and coalesce blocks properly.

### **Files Created:**

* `alloc.h`
* `alloc.c`
* `test_alloc.c`

### **alloc.h**

```c
#ifndef ALLOC_H
#define ALLOC_H

int init_alloc(void);
int cleanup_alloc(void);
char *alloc_mem(int size);
void dealloc_mem(char *ptr);

#endif // ALLOC_H
```

### **alloc.c**

This file implements the core allocator logic using:

* `mmap()` for page allocation
* A **free list** (linked list) to track available space
* A **first-fit** strategy for allocation
* Coalescing adjacent free blocks on `dealloc_mem()`

```c
#define PAGE_SIZE 4096

typedef struct FreeNode {
    size_t offset;
    size_t size;
    struct FreeNode *next;
} FreeNode;

typedef struct AllocNode {
    char *ptr;
    size_t size;
    struct AllocNode *next;
} AllocNode;
```

Key functions:

* `init_alloc()` → calls `mmap()` to reserve 4KB.
* `alloc_mem(size)` → allocates space using first-fit.
* `dealloc_mem(ptr)` → returns space and merges free chunks.
* `cleanup_alloc()` → unmaps the page and releases metadata.

### **test_alloc.c**

Simple test program that allocates, frees, and reallocates blocks to verify correctness.

```bash
gcc alloc.c test_alloc.c -o test_alloc
./test_alloc
```

---

## 🚀 Task 2: Elastic Allocator (`ealloc`)

### **Goal:**

Implement a scalable allocator that:

* Initially has **no pages mapped**.
* Dynamically requests new pages using `mmap()` when existing pages fill up.
* Allows multiple pages (up to 4 by default, but configurable).
* Handles allocations in multiples of **256 bytes**.

### **Files Created:**

* `ealloc.h`
* `ealloc.c`
* `test_ealloc.c`

### **ealloc.h**

```c
#ifndef EALLOC_H
#define EALLOC_H

int einit_alloc(void);
int ecleanup_alloc(void);
char *ealloc_mem(int size);
void edealloc_mem(char *ptr);

#endif // EALLOC_H
```

### **ealloc.c**

The allocator maintains a linked list of `PageDesc` structures, each representing one 4KB memory page:

```c
typedef struct PageDesc {
    char *base;
    struct FreeNode *free_list;
    struct PageDesc *next;
} PageDesc;
```

Each page tracks its own free list and allocation map. When all existing pages are full, a new page is `mmap()`'ed and appended to the list.

### **Key Functions:**

* `einit_alloc()` → Initializes metadata structures.
* `ealloc_mem(size)` → Allocates from an existing or newly created page.
* `edealloc_mem(ptr)` → Frees memory and coalesces within a page.
* `ecleanup_alloc()` → Frees metadata structures (but leaves pages mapped, per assignment note).

### **test_ealloc.c**

Simple verification program that tests page creation and reusability of freed memory.

```bash
gcc ealloc.c test_ealloc.c -o test_ealloc
./test_ealloc
```

---

## 🧠 Concepts Demonstrated

1. **Memory Mapping:** Using `mmap()` and `munmap()` to manage virtual memory directly.
2. **Free List Management:** Implementing dynamic linked lists for free and allocated blocks.
3. **First-Fit Allocation Strategy:** Choosing the first suitable block from the free list.
4. **Coalescing Free Blocks:** Merging adjacent freed memory regions to prevent fragmentation.
5. **Dynamic Expansion:** Handling multiple pages efficiently in user-space.

---

## ⚙️ Compilation and Execution

```bash
# Task 1
gcc -Wall -Wextra alloc.c test_alloc.c -o test_alloc
./test_alloc

# Task 2
gcc -Wall -Wextra ealloc.c test_ealloc.c -o test_ealloc
./test_ealloc
```

---

## ✅ Expected Output Example

```
hello from a
b-size:128
allocated d at 0x7f...
Done
hello elastic
d allocated at 0x7f...
elastic done
```

---

## 📂 Deliverables

* `alloc.h`, `alloc.c`, `test_alloc.c`
* `ealloc.h`, `ealloc.c`, `test_ealloc.c`
* `README` (this document)

---

**End of Report**