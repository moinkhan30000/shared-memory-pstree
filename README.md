# Shared-Memory Process Tree (PSTree) Library

## Overview
A C library (`libpstree`) that provides a shared binary search tree (BST) abstraction for multiple concurrent processes. The tree is stored entirely within a shared memory region, allowing disparate processes to dynamically allocate nodes, insert key-value pairs, and query data simultaneously without race conditions.

## Technical Details
* **Environment:** C, Linux, POSIX Shared Memory
* **Concurrency:** Implements POSIX Pthreads mutexes and condition variables mapped directly into shared memory to ensure thread-safe tree modifications across process boundaries.
* **Memory Management:** Features custom dynamic memory allocation strategies to manage node creation and deletion strictly within the shared `mmap` region, bypassing standard `malloc()`.

## Build & Execution
Compiled and tested on Ubuntu 22.04 (x86-64).

```bash
# Build the library and all concurrent test applications
make

# Execute concurrent test apps to verify synchronization
./app1-add
./app2-get
```

## Concurrency Testing & Analysis
The synchronization logic was rigorously validated on an Ubuntu 22.04 LTS (64-bit) 8-core virtual machine with 8 GB of RAM[cite: 11]. A full concurrency analysis is available in `Performance_Analysis.pdf`.

**Test Harness Architecture**
The test suite utilizes seven standalone C programs (`app1-add`, `app1-get`, `app2-insert`, `app2-get`, `app3-insert-delete`, `app3-get`, `app4-mixed`) running concurrently to execute simultaneous insert, delete, and read operations[cite: 11].

**Synchronization Integrity**
* **Strict Mutual Exclusion:** All tree modifications and traversals are serialized using a single `PTHREAD_PROCESS_SHARED` mutex stored directly in the shared memory header[cite: 11].
* **Zero Busy-Waiting:** The library ensures blocked threads sleep inside the kernel instead of relying on CPU-intensive spin loops[cite: 11].
* **Stability:** Testing confirmed zero instances of data corruption, duplicate keys, or unexpected crashes during simultaneous multi-process access once the shared segment was initialized[cite: 11].

**Startup Race Mitigation**
During testing, an initialization race condition was identified where consumer processes attempted to map the memory before the creator process completed the `shm_open`, `ftruncate`, and `mmap` sequence, resulting in `PST_ERROR` returns[cite: 11]. 
* **Test Fix:** Mitigated in the testing harness using sequential `sleep 2` delays to guarantee the memory layout was initialized before concurrent access began[cite: 11]. 
* **Production Recommendation:** For production-grade resilience, the analysis recommends replacing fixed sleeps with retry loops inside `pst_open`, named semaphores, or a dependency-aware launcher script[cite: 11].
