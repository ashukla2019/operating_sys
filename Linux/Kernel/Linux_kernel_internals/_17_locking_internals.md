# Chapter 17 – Locking Internals (qspinlock, mutex, rwsem, seqlock, futex, PREEMPT_RT)

> Extends `7_synchronization.md` §6-26. Priority: ★★★★★

---

## 1. Lock Taxonomy (Linux)
| Lock | Sleeps? | Use |
|---|---|---|
| `raw_spinlock_t` | no | lowest level, always spins (even on RT) |
| `spinlock_t` | no (sleeps on PREEMPT_RT) | short sections, IRQ/softirq paths |
| `mutex` | yes | process ctx, longer sections |
| `rw_semaphore` | yes | read-mostly, sleepable (e.g. `mmap_lock`) |
| `rwlock_t` | no | rarely recommended now |
| `seqlock_t` / `seqcount_t` | no | tiny read-mostly data, readers retry |
| `percpu_rw_semaphore` | yes | extremely read-heavy, rare writers |
| `semaphore` | yes | legacy / counting |
| `local_lock_t` | no | protect per-CPU data (RT-friendly) |
| `bit spinlock` | no | lock bit in a word (`bit_spin_lock`) |

## 2. Spinlock Basics
- `spin_lock()` = `preempt_disable()` + acquire lock → holder **cannot be preempted** → bounded waits
- `_irq`, `_irqsave`, `_bh` variants: prevent same-CPU re-entry from IRQ / softirq on the same lock
- Rule: if the lock is ever taken in IRQ context, **every** process-context taker must disable IRQs
- Don't sleep, don't call `copy_to_user`, `kmalloc(GFP_KERNEL)`, `mutex_lock` while holding
- Keep it short; avoid nested locks

## 3. Why Simple Test-and-Set Spinlock Is Bad
- All waiters spin on the **same cache line** doing atomic RMW → coherence storm
- **Unfair**, starvation possible
- TTAS (test, then test-and-set) helps spinning but still herds on release

## 4. Ticket Lock (old default, until ~4.2 x86 / 4.17 arm64)
- `next` and `owner` counters; take ticket → spin until `owner == my ticket` → FIFO fairness
- Still all waiters spin on **one** cache line (`owner`) → O(N) invalidations on every unlock
- Larger lock word (8 bytes) → hurts embedded structs

## 5. qspinlock (current default) – 4 bytes!
- Word layout: `locked` byte | `pending` bit | `tail` (CPU id + nesting index)
- **Fast path**: `cmpxchg(0 → LOCKED)`
- **Contended, 1 waiter**: set **pending** bit and spin on the lock word (no queue allocation)
- **Contended, ≥2 waiters**: enqueue in **MCS queue**; each waiter spins on **its own per-CPU node** (local cache line); only queue head spins on lock word
- Unlock: just clear locked byte; next waiter wakes by writing its node's flag → O(1) cache traffic
- **4 per-CPU MCS nodes** – for task, softirq, hardirq, NMI nesting (a CPU can wait on at most 4 spinlocks at once)
- Fair-ish (FIFO among queued), scalable, small
- **Paravirt qspinlock** (`pv_wait/pv_kick`): in VMs waiters halt vCPU instead of burning cycles (fixes **lock-holder preemption**)
- ARM64 uses qspinlock too (with `wfe`/`sevl` for low-power waiting)

## 6. Mutex Internals
- `struct mutex { atomic_long_t owner; spinlock_t wait_lock; struct list_head wait_list; ... }`
- `owner` holds `task_struct *` + low-bit flags (`WAITERS`, `HANDOFF`, `PICKUP`)
- **Fast path**: `cmpxchg(owner, 0, current)` for lock; `cmpxchg(owner, current, 0)` for unlock
- **Mid path – optimistic spinning**: if owner is **running on another CPU**, spin briefly (via **OSQ**, an MCS-style queue so only one spinner per mutex) instead of sleeping → avoids sleep/wakeup cost for short holds
  - Stop spinning if owner sleeps, or `need_resched()`
- **Slow path**: add to `wait_list`, `schedule()`; on unlock wake first waiter; `HANDOFF` prevents starvation of sleepers by lock stealers
- Rules: only owner may unlock, no recursive lock, not in IRQ/softirq, no exit with lock held, `mutex_trylock` ok but not in IRQ
- Debug: `CONFIG_DEBUG_MUTEXES`, lockdep, `/proc/<pid>/stack`, `hung_task` shows `mutex_lock` owner

## 7. rw_semaphore (rwsem)
- `count` + `owner` + `wait_list`; sleepable, supports **optimistic spinning** (writers, and readers in some cases)
- Reader-writer fairness: writers queued block new readers (avoids writer starvation); "lock stealing" for throughput
- Downgrade `downgrade_write()` supported; `down_write_killable()` for fatal-signal-aware
- Famous user: **`mmap_lock`** (was `mmap_sem`)
  - Page faults take read; `mmap/munmap/mprotect/brk` take write → big scalability bottleneck for multithreaded apps
  - Mitigations: **per-VMA locks** (6.4+), **speculative page fault** work, maple tree (6.1) replacing rbtree+linked list
- Why not `rwlock_t`? Readers still write the shared counter; both have cache line bouncing; rwsem at least can sleep

## 8. percpu_rw_semaphore
- Readers: increment **per-CPU** counter (no shared write) when no writer pending
- Writer: announces via `rcu_sync`, waits for readers via GP, then takes exclusive → **writers very expensive**
- Use when writers are very rare (e.g. `cgroup_threadgroup_rwsem`, filesystem freeze, uprobes)

## 9. seqlock / seqcount
- Writer: `write_seqlock()` → seq++ (odd) → modify → seq++ (even)
- Reader: `do { s = read_seqbegin(&sl); copy data; } while (read_seqretry(&sl, s));`
- Readers never block writers; writers never wait for readers; readers **retry**
- Constraints: data must be **copyable** (no following pointers that might be freed), writers serialised
- Users: `jiffies_64`/timekeeping (`xtime`), `d_seq` in dcache, `mount_lock`, `/proc` stats
- `seqcount_latch_t` for NMI-safe readers; `seqcount_LOCKNAME_t` ties seqcount to a lock for lockdep/RT

## 10. Semaphore (counting)
- `down()` / `up()`; `struct semaphore { raw_spinlock_t lock; unsigned count; list wait_list }`
- No ownership → no PI → no lockdep deadlock detection; prefer `mutex` / `completion`

## 11. Futex (user ↔ kernel) – the base of pthread mutex/condvar
- Idea: lock word in **user memory**; uncontended path = atomic op, **no syscall**
- Contended: `futex(addr, FUTEX_WAIT, expected_val)` – kernel checks `*addr == expected` **under hash-bucket lock**, then sleeps; `FUTEX_WAKE(addr, n)` wakes n
- The value re-check under the bucket lock is what prevents the **lost-wakeup race**
- Kernel side: global hash table of `futex_hash_bucket`; key = (mm, addr) private, or (inode, offset) shared; `struct futex_q` per waiter
- `FUTEX_PRIVATE_FLAG` skips mm/inode lookup → faster
- Other ops: `FUTEX_REQUEUE/CMP_REQUEUE` (condvar broadcast without thundering herd), `FUTEX_WAIT_BITSET`, **PI futexes** (`FUTEX_LOCK_PI`, backed by `rt_mutex`), `futex_waitv` (futex2, 5.16+; wait on multiple, used by Wine/Proton)
- **Robust futexes**: kernel walks thread's robust list at exit → marks `FUTEX_OWNER_DIED` → other waiters recover (`EOWNERDEAD`)
- 3-state mutex (Drepper): 0 unlocked / 1 locked / 2 locked+waiters → unlock only syscalls if state was 2

## 12. Priority Inversion & Priority Inheritance
- Low-prio L holds lock; high-prio H blocks; medium M preempts L → H starved (Mars Pathfinder)
- **PI**: L temporarily boosted to H's priority until unlock
- Kernel: `rt_mutex` (PI-aware), `PI futex`, `PTHREAD_PRIO_INHERIT`
- Regular `mutex` has no PI (non-RT tasks use nice/fair, so less critical)

## 13. PREEMPT_RT (mainline since 6.12)
- Goal: bounded worst-case latency – almost everything preemptible
- `spinlock_t`, `rwlock_t` → **rt_mutex-based sleeping locks** with PI
- `raw_spinlock_t` remains a real spinlock (scheduler, IRQ core, low-level)
- Hard IRQ handlers become **threaded** (`threadirqs`), softirqs run in thread context
- `local_irq_disable()` no longer means "no preemption" → use `local_lock` for per-CPU data
- Consequences for drivers: can't assume `spin_lock` disables preemption; careful with `preempt_disable` regions and `raw_spinlock`
- Preemption models: `none`, `voluntary`, `full`, `RT` (+ lazy preemption in newer kernels)

## 14. preempt_count (what "atomic context" really is)
- Per-task counter with fields: **preempt depth**, **softirq count**, **hardirq count**, **NMI count**
- `in_interrupt()`, `in_atomic()`, `in_task()` read it
- `might_sleep()` + `CONFIG_DEBUG_ATOMIC_SLEEP` → "BUG: sleeping function called from invalid context"
- `spin_lock` ⇒ `preempt_disable` ⇒ `preempt_count++`

## 15. Lockless Building Blocks
- `llist` (lock-free singly linked, multi-producer/single-consumer `llist_del_all`)
- `kfifo` (SPSC without lock), `ptr_ring`, `xarray`/`radix tree` (RCU-friendly lookup)
- `cmpxchg` loops, `atomic_try_cmpxchg`
- ABA problem: pointer reused between read and CAS → mitigate with tagged pointers / RCU / hazard pointers / not freeing during window
- "Lockless ≠ waitless" – retries and cacheline bouncing still cost

## 16. Lockdep
- Tracks lock **classes** (not instances), builds dependency graph A→B, detects **potential** cycles even if deadlock hasn't occurred
- Also checks **IRQ-safety rules** (lock taken in IRQ but acquired without irq-disable elsewhere), unlock imbalance, held-lock at return to user
- Annotations: `lockdep_assert_held()`, `mutex_lock_nested()`, `lockdep_set_class()`, `lockdep_set_novalidate_class`
- Output: "possible circular locking dependency detected", "inconsistent lock state", "possible recursive locking"
- Runtime cost: significant – debug kernels only. `/proc/lockdep_stats`, `/proc/lock_stat` (contention stats)

## 17. Contention Analysis
- `perf lock record/report`, `lock_stat`, `bpftrace` on `lock:contention_begin`, off-CPU flame graphs
- Symptoms: high `%sys`, many spin samples in `queued_spin_lock_slowpath`, `osq_lock`, `rwsem_down_*`
- Fixes: **reduce hold time**, **shard** (per-bucket, per-CPU, per-node), **RCU** for reads, **batching**, change data structure, avoid lock in hot path (e.g. `percpu_counter`)
- Amdahl: serial fraction under lock bounds speedup

## 18. Lock-Holder Preemption & Virtualisation
- vCPU holding a spinlock is descheduled by hypervisor → other vCPUs spin uselessly
- Mitigations: paravirt spinlocks, PLE (pause-loop exit) on Intel / PF on AMD, `kvm` yield-to

## 19. Choosing a Lock – Decision Table
```
Can sleep & long?           -> mutex / rwsem
IRQ / softirq involved?     -> spinlock_irqsave / spin_lock_bh
Simple counter/flag?        -> atomic_t / bitops (mind ordering)
Per-CPU stats?              -> per-CPU vars / percpu_counter
Read-mostly, pointers?      -> RCU
Read-mostly, small copyable -> seqlock
Super read-heavy, rare wr.  -> percpu_rw_semaphore
Wait for event?             -> completion / waitqueue
User+kernel fast path?      -> futex
```

---

## Senior Interview Questions

**Q1. Why did Linux replace ticket spinlocks with qspinlock?**
- Ticket: all waiters spin on one cache line, 8B size. qspinlock: 4B, MCS queue with per-CPU spin variables → O(1) coherence per handoff.

**Q2. How does qspinlock handle one waiter vs many?**
- One waiter: pending bit. More: MCS queue; head spins on lock word, others on private node.

**Q3. Explain mutex optimistic spinning.**
- If owner is running on another CPU, spin (OSQ-limited) since release is likely soon; cheaper than sleep+wakeup. Stop if owner blocks or we need to reschedule.

**Q4. Spinlock held and code calls `kmalloc(GFP_KERNEL)` – what happens?**
- Can sleep in reclaim → "sleeping function called from invalid context" / possible deadlock. Use `GFP_ATOMIC` or allocate before taking lock.

**Q5. Why does `spin_lock_irqsave` store flags?**
- Restore previous IRQ state (nested use), rather than unconditionally enabling.

**Q6. How does futex avoid syscalls?**
- Atomic op on user word in fast path; syscall only when contended (WAIT) or when waiters exist (WAKE).

**Q7. How is the futex lost wakeup prevented?**
- Kernel re-checks the user value under the bucket lock before queueing; waker takes same bucket lock.

**Q8. Seqlock vs RCU?**
- Seqlock: readers retry, data copied, can't chase pointers safely. RCU: readers never retry, pointers valid until GP.

**Q9. Why is `mmap_lock` a scalability problem?**
- Single rwsem per mm: page faults (read) vs mmap/munmap (write) contend; reader count cacheline bounces; per-VMA locks reduce.

**Q10. What changes with PREEMPT_RT?**
- `spinlock_t` becomes sleeping PI lock, IRQs threaded; need `raw_spinlock_t` for truly atomic sections; `local_lock` for per-CPU data.

**Q11. What does lockdep detect and not detect?**
- Detects potential ordering cycles, IRQ-context misuse, recursion. Not: livelock, logic races without locks, lock-free bugs (KCSAN for data races).

**Q12. Two threads show 100% CPU in `queued_spin_lock_slowpath` – approach?**
- `perf record -g` find which lock/caller, `lock_stat`; shorten/shard/replace; check hold time and NUMA effect.

**Q13. Priority inversion: how does Linux solve it?**
- `rt_mutex` PI, PI futexes, `PTHREAD_PRIO_INHERIT`; RT throttling as safety net.

---

## Quick Revision
```
spinlock : preempt_disable + qspinlock (pending bit -> MCS queue), irqsave if used in IRQ
mutex    : cmpxchg owner; optimistic spin (OSQ); sleep w/ handoff
rwsem    : sleepable rw, mmap_lock; percpu_rwsem = rare writers
seqlock  : writer odd/even seq, readers retry; no pointers
futex    : user atomic fast path, WAIT/WAKE slow path, PI via rt_mutex
RT       : spinlock_t -> sleeping PI lock; raw_spinlock_t stays
lockdep  : class graph, finds POTENTIAL deadlock
```
