# Chapter 22 – User-Space Multithreading (pthreads, C++ atomics, Lock-Free, Patterns)

> Companion to Ch. 7, 15-17. Priority: ★★★★★ (the "Multi-threading" half of your target topic)

---

## 1. Threads in Linux (recap)
- Thread = `clone()` with `CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD | CLONE_SETTLS | CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID`
- NPTL: 1:1 model; each pthread = a kernel task (`tid`), same `tgid`
- `CLONE_CHILD_CLEARTID` → kernel zeroes tid word + `futex_wake` at exit → **how `pthread_join` works**
- Each thread has its own: stack, TLS, signal mask, errno, sched policy/affinity, `task_struct`
- Shared: address space, FD table, signal handlers, cwd

## 2. Stack & TLS
- Default stack 8 MB (virtual, lazily committed); `pthread_attr_setstacksize`; guard page at bottom
- TLS models: **initial-exec / local-exec** (fast, `%fs`/`TPIDR_EL0` relative) vs **general/local-dynamic** (`__tls_get_addr`, for `dlopen`)
- `thread_local` (C++11) / `__thread` / `_Thread_local`; destructors run at thread exit

## 3. pthread Mutex
- Types: `NORMAL`, `RECURSIVE`, `ERRORCHECK`, `ADAPTIVE_NP` (spin then sleep)
- Attributes: **`PTHREAD_PROCESS_SHARED`** (in shared memory across processes), **robust** (`pthread_mutex_consistent`, `EOWNERDEAD`), **priority protocol** (`PTHREAD_PRIO_INHERIT/PROTECT`)
- Implementation: futex; fast path atomic CAS; contended → `FUTEX_WAIT`
- Unlock by non-owner: UB (`ERRORCHECK` returns `EPERM`)
- `pthread_mutex_trylock`, `timedlock`
- Avoid holding across blocking IO/syscalls where possible

## 4. Condition Variables
```c
pthread_mutex_lock(&m);
while (!predicate)            // ALWAYS a loop: spurious wakeups + stolen wakeups
    pthread_cond_wait(&cv, &m);   // atomically unlock+sleep; relocks on return
consume();
pthread_mutex_unlock(&m);
```
- Producer: change state under lock, `signal`/`broadcast` (before or after unlock; after unlock avoids "hurry up and wait" but both correct)
- Why spurious wakeups: futex requeue / signals / implementation freedom; also multiple consumers
- `pthread_cond_timedwait` uses absolute time (`CLOCK_REALTIME` default; set `pthread_condattr_setclock(CLOCK_MONOTONIC)`)
- Lost wakeup = signalling before waiter checks predicate without state change under lock

## 5. Other Primitives
- `pthread_rwlock_t` (reader/writer preference attributes; writer starvation risk), `pthread_spinlock_t` (user spinning is dangerous: preempted holder → waste; prefer mutex/adaptive)
- `pthread_barrier_t`, `sem_t` (POSIX semaphore, futex-based), `pthread_once`, `call_once`
- C++: `std::mutex`, `shared_mutex`, `condition_variable`, `scoped_lock` (deadlock-free multi-lock via `std::lock`), `unique_lock`, `latch`, `barrier`, `counting_semaphore`, `jthread`, `stop_token`
- Linux-specific: `eventfd`, `futex`, `epoll`, `io_uring`, `signalfd`, `timerfd`

## 6. C++ Memory Model & `std::atomic`
| Order | Guarantee |
|---|---|
| `relaxed` | atomicity only (counters, stats) |
| `acquire` (load) | later ops can't move before |
| `release` (store) | earlier ops can't move after |
| `acq_rel` (RMW) | both |
| `seq_cst` (default) | single total order of all seq_cst ops |
| `consume` | dependency ordering (in practice treated as acquire) |
- **Synchronizes-with**: release store → acquire load of same atomic that reads that value → happens-before
- `compare_exchange_weak` (may fail spuriously, use in loops; cheaper on LL/SC) vs `_strong`
- `std::atomic<T>::is_lock_free()`; `atomic<shared_ptr>` (C++20); `atomic_flag` (only guaranteed lock-free type)
- `std::atomic_thread_fence` – standalone fences
- x86: `seq_cst` store = `xchg`; ARM64: `stlr`/`ldar`; `fetch_add` = `lock xadd` / `ldadd` (LSE) or `ldxr/stxr` loop
- **Data race = undefined behaviour**; `volatile` is NOT synchronization

## 7. Classic Patterns & Code
**Spinlock (TTAS with backoff)**
```cpp
struct Spin {
  std::atomic<bool> f{false};
  void lock(){ for(;;){ if(!f.exchange(true, std::memory_order_acquire)) return;
                         while(f.load(std::memory_order_relaxed)) cpu_relax(); } }
  void unlock(){ f.store(false, std::memory_order_release); }
};
```
**SPSC ring buffer (lock-free, wait-free)**
```cpp
// capacity power of two; head owned by consumer, tail by producer
bool push(T v){ auto t=tail.load(relaxed); if(t-head.load(acquire)==N) return false;
                buf[t&(N-1)]=v; tail.store(t+1, release); return true; }
bool pop(T&v){ auto h=head.load(relaxed); if(h==tail.load(acquire)) return false;
               v=buf[h&(N-1)]; head.store(h+1, release); return true; }
// put head and tail on separate cache lines (alignas(64)); cache local copies of the other index to reduce traffic
```
**Futex-based mutex (Drepper, 3-state)**
```
0 = unlocked, 1 = locked no waiters, 2 = locked with waiters
lock:   c = cmpxchg(0->1); if c!=0 { if c!=2: c=xchg(2); while c!=0 { futex_wait(2); c=xchg(2);} }
unlock: if atomic_dec(state)!=1 { state=0; futex_wake(1);}
```
**Bounded blocking queue**: mutex + 2 condvars (`not_full`, `not_empty`) with predicate loops
**Thread-safe singleton**: Meyers `static T& get(){static T t; return t;}` (C++11 thread-safe) or `call_once`; double-checked locking only with acquire/release atomics

## 8. Lock-Free Data Structures
- Progress guarantees: **obstruction-free < lock-free < wait-free**
- Treiber stack: CAS on head — suffers **ABA**
- Michael-Scott queue: two CAS (head/tail) with dummy node
- **ABA** mitigation: tagged/versioned pointers (double-width CAS `cmpxchg16b` / `casp`), hazard pointers, epoch-based reclamation (EBR), RCU (liburcu), reference counting, never reuse nodes
- **Safe memory reclamation** is the hard part, not the CAS
- MPMC bounded queue (Vyukov) – per-slot sequence numbers
- Combining techniques, flat combining, sharding, per-thread caches
- Reality: prefer simple locks + sharding; lock-free only when profiled benefit

## 9. Performance & Scalability
- **Amdahl's law**: speedup ≤ 1 / (s + (1−s)/N); **Universal Scalability Law** adds coherence cost
- Kill contention: shard, per-thread/per-CPU data, batching, read-copy-update, reduce critical section, `try_lock` + fallback
- **False sharing**: `alignas(std::hardware_destructive_interference_size)` / 64; check with `perf c2c`
- **Cache-line bouncing** on shared counters → per-thread counters aggregated lazily
- **Lock convoy**: threads repeatedly queue behind a descheduled holder
- **Thundering herd**: many waiters woken for one item → `EPOLLEXCLUSIVE`, `SO_REUSEPORT`, `FUTEX_REQUEUE`, wake-one
- **Priority inversion**: PI mutex or avoid sharing between priorities
- **NUMA**: allocate per-node, pin threads (`pthread_setaffinity_np`), first-touch
- Allocator: glibc per-thread **arenas** + `tcache`; jemalloc/tcmalloc/mimalloc; many arenas → memory bloat

## 10. Thread Pool Design
- Components: worker threads, task queue (global vs per-worker deques + **work stealing**), shutdown protocol, exception propagation, backpressure (bounded queue), futures/promises
- Sizing: CPU-bound ≈ #cores; IO-bound ≈ cores × (1 + wait/compute)
- Pitfalls: nested submit deadlock (workers waiting on tasks queued behind them), unbounded queue memory growth, false sharing on queue head, wake-up latency (use spin-then-park)
- Alternatives: event loop (epoll/io_uring) + thread-per-core, coroutines (C++20), actor model

## 11. Signals + Threads + fork
- Signal delivered to **any thread** not blocking it (process-directed) or specific thread (`pthread_kill`, synchronous faults)
- Pattern: block signals in all threads, dedicate one thread with `sigwait`/`signalfd`
- Handlers may call only **async-signal-safe** functions (not `malloc`, `printf`, `pthread_mutex_lock`)
- `fork()` in multithreaded program: child has **only the calling thread**; locks held by other threads stay locked forever (malloc, stdio) → only async-signal-safe calls until `exec`; `pthread_atfork` handlers; prefer `posix_spawn`
- `pthread_cancel`: deferred vs async; cleanup handlers; avoid – use cooperative stop flags (`stop_token`)

## 12. Deadlock/Livelock/Starvation
- Four conditions (mutual exclusion, hold-and-wait, no preemption, circular wait) – break one
- Prevent: lock hierarchy, `std::scoped_lock(a,b)`, try-lock with back-off, lock-free design
- Detect: `gdb thread apply all bt`, `pstack`, TSAN lock-order inversion, `helgrind`, `pthread_mutex` debug, `eu-stack`
- **Livelock**: threads keep retrying without progress (fix: randomized backoff). **Starvation**: unfair locks / reader preference

## 13. Tools
| Tool | Use |
|---|---|
| ThreadSanitizer (`-fsanitize=thread`) | data races, lock-order inversions |
| Helgrind/DRD (valgrind) | races/deadlocks (slow) |
| `perf` + `perf c2c`, `perf lock` | contention, false sharing |
| `strace -f -T`, `ltrace` | futex storms |
| `gdb` / `rr` | inspect threads, deterministic replay |
| `pidstat -wt`, `top -H` | per-thread CPU/ctx switches |
| `/proc/<pid>/task/<tid>/{stat,stack,status}` | per-thread state/wchan |
| eBPF `offcputime`, `futex` tracing | where threads block |

## 14. Coding Questions to Practice (write them from scratch!)
1. Producer/consumer with bounded buffer (mutex+condvar, then semaphores)
2. Print odd/even / ABC in order using N threads
3. Reader-writer lock (reader-preferring, then writer-preferring)
4. Dining philosophers (resource hierarchy, `scoped_lock`)
5. Thread-safe LRU cache (sharded locks)
6. Barrier implementation (generation counter + condvar)
7. SPSC ring buffer with acquire/release
8. Semaphore from mutex+condvar
9. Thread pool with futures, graceful shutdown
10. Lock-free stack + ABA explanation
11. Rate limiter / token bucket with atomics
12. Implement a spinlock + ticket lock and compare under contention

---

## Senior Interview Questions

**Q1. How does `pthread_mutex_lock` work without syscalls in the uncontended case?**
- Atomic CAS on user word; syscall only on contention via futex.

**Q2. Why must condvar waits be in a `while` loop?**
- Spurious wakeups, stolen wakeups by other consumers; predicate must be rechecked under lock.

**Q3. `std::memory_order_relaxed` counter: when safe?**
- Pure statistics where no other data depends on the value.

**Q4. Explain release/acquire with an example.**
- Producer writes data then `flag.store(1, release)`; consumer `flag.load(acquire)==1` then reads data safely.

**Q5. What is ABA and how to avoid?**
- Value changes A→B→A hiding a change from CAS; use versioned pointers, hazard pointers/EBR/RCU.

**Q6. Lock-free vs wait-free?**
- Lock-free: system-wide progress; wait-free: every thread finishes in bounded steps.

**Q7. What happens to locks in the child after `fork()` in a multithreaded process?**
- Locks held by absent threads remain locked; only async-signal-safe functions allowed before `exec`.

**Q8. How do you find why a multithreaded service is slow with 64 threads?**
- `perf top/record -g`, off-CPU analysis, lock contention (`perf lock`/futex tracing), `perf c2c`, check scheduler latency, NUMA, allocator arenas, then shard/reduce.

**Q9. How to size a thread pool?**
- CPU-bound: ~cores; IO-bound: cores × (1+W/C); validate with load tests; consider event loop.

**Q10. `volatile` for thread sync?**
- No: no atomicity or ordering guarantees; use `std::atomic`.

**Q11. How to implement a read-mostly shared config in userspace without locks?**
- `atomic<shared_ptr>` or RCU (liburcu); readers load pointer; writers swap & defer reclaim.

**Q12. What is a priority-inversion-safe user mutex?**
- `PTHREAD_PRIO_INHERIT` (PI futex via `rt_mutex`).

---

## Quick Revision
```
Thread    : clone(CLONE_THREAD...), futex for join/mutex/cond
Condvar   : while(!pred) wait(); signal state changes under lock
Atomics   : relaxed < acquire/release < seq_cst; CAS loops; weak vs strong
Lock-free : ABA + memory reclamation (hazard ptr / EBR / RCU)
Scale     : shard, per-thread data, avoid false sharing, NUMA-pin
fork+threads: only async-signal-safe until exec
Tools     : TSAN, perf c2c/lock, gdb thread apply all bt
```
