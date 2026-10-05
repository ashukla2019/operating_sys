# Chapter 16 – Memory Ordering, Atomics & Cache Coherency

> Extends `7_synchronization.md` §27-32, §58-59. Priority: ★★★★★ (ARM / Qualcomm / Broadcom love this; x86 vs ARM64 is a favourite question)

---

## 1. Why Reordering Happens
- **Compiler**: reorders / merges / re-loads / tears accesses for optimisation
- **CPU**: out-of-order execution, store buffers, speculative loads, invalidate queues
- **Cache hierarchy**: different cores see writes at different times
- Single-thread semantics preserved; **cross-thread visibility is not**

## 2. Cache Coherency vs Memory Ordering
- **Coherency**: all cores agree on the order of writes to **one address** (hardware guarantees)
- **Ordering / consistency**: order in which writes to **different addresses** become visible
- Coherency ≠ ordering – this distinction is a classic question

## 3. MESI / MOESI / MESIF
| State | Meaning |
|---|---|
| M Modified | only copy, dirty |
| E Exclusive | only copy, clean (silent upgrade to M) |
| S Shared | multiple clean copies |
| I Invalid | not present |
| O Owned (MOESI – AMD) | dirty but shared; owner supplies data |
| F Forward (MESIF – Intel) | designated responder among sharers |
- Write to S line → send invalidate (RFO) → other copies → I
- Cost of contended atomic = cache line ping-pong between cores (hundreds of cycles cross-socket)
- Snoop-based (small) vs directory-based (large / multi-socket); ARM uses CHI/ACE interconnect

## 4. Store Buffer & Invalidate Queue (why reordering exists)
- Store buffer: writes retire into buffer, drained later → **a later load can pass an earlier store** (StoreLoad reorder, even on x86)
- Invalidate queue: invalidate acks are queued → stale reads possible
- Barriers drain the buffer / process the queue

## 5. x86-TSO vs ARM64 (weak)
| Reordering | x86 (TSO) | ARM64 / POWER / RISC-V(RVWMO) |
|---|---|---|
| Load → Load | no | **yes** |
| Load → Store | no | **yes** |
| Store → Store | no | **yes** |
| Store → Load | **yes** (store buffer) | **yes** |
| Dependent loads | ordered | ordered (address dep.) |
- ARMv8: "other-multi-copy atomic" – a store becomes visible to all other observers simultaneously
- Consequence: code that works on x86 by luck breaks on ARM64 → **always write to the language/kernel memory model, not to the hardware**

## 6. Classic Litmus Tests
- **Message passing (MP)** – fix with release on flag store, acquire on flag load
```
P0: data = 1;           P1: while(!flag);
    flag = 1;               r = data;     // may read 0 on ARM without ordering
```
- **Store buffering (SB / Dekker)** – both threads may read 0 even on x86 → needs full barrier (`smp_mb`/seq_cst)
- **Load buffering (LB)**, **IRIW** (independent reads of independent writes – multi-copy atomicity)
- Tools: `herd7`, kernel `tools/memory-model` (LKMM litmus tests), CBMC, `litmus7`

## 7. Kernel Memory Barrier API
| API | Meaning | ARM64 impl | x86 impl |
|---|---|---|---|
| `barrier()` | compiler only | – | – |
| `smp_mb()` | full | `dmb ish` | `lock; addl` / `mfence` |
| `smp_rmb()` | load-load | `dmb ishld` | compiler barrier |
| `smp_wmb()` | store-store | `dmb ishst` | compiler barrier |
| `smp_load_acquire()` | later accesses can't move before | `ldar` | plain load |
| `smp_store_release()` | earlier accesses can't move after | `stlr` | plain store |
| `smp_mb__before/after_atomic()` | order around non-returning atomics | dmb | compiler/none |
| `mb()/rmb()/wmb()` | **mandatory** barriers (incl. devices) | `dsb` | `mfence` etc. |
| `dma_rmb()/dma_wmb()` | CPU ↔ coherent DMA memory | `dmb oshld/oshst` | compiler |
- `smp_*` compile to nothing on UP; `mb()` don't

## 8. Acquire / Release (the useful abstraction)
- **Release store**: all earlier memory ops complete before this store is visible
- **Acquire load**: no later memory op can be hoisted above this load
- Pair them on the **same variable** → "synchronizes-with" → happens-before
- Lock = acquire on lock, unlock = release → critical sections can't leak outwards
- Cheaper than full barrier (esp. ARM64: `ldar/stlr`)

## 9. READ_ONCE / WRITE_ONCE
- Prevent compiler: **tearing**, **fusing** (merging two loads), **re-loading** (load twice, get two values), **invented loads/stores**
- Required for any lockless shared access: `while (!READ_ONCE(flag)) cpu_relax();`
- They do **not** add hardware ordering by themselves
- `data_race(x)` – annotate intentional racy reads (KCSAN)
- Rule of thumb: plain C access to shared data without a lock = **data race = undefined behaviour**

## 10. Dependencies
- **Address dependency** (load p, then load *p): ordered → basis of `rcu_dereference`
- **Data dependency** (load → store of that value): ordered
- **Control dependency** (load → branch → store): orders load→store only, not load→load; compiler can break it → avoid unless using `READ_ONCE` + care
- Compiler can break dependencies by value speculation (hence `rcu_dereference` rules)

## 11. Linux Atomic API
- Types: `atomic_t` (int), `atomic64_t`, `atomic_long_t`, `refcount_t`, bitops (`set_bit`, `test_and_set_bit`)
- **Ordering rules (important!)**
  - Non-value-returning (`atomic_inc`, `atomic_add`, `set_bit`) → **no ordering**
  - Value-returning (`atomic_add_return`, `atomic_cmpxchg`, `test_and_set_bit`) → **fully ordered**
  - Suffixes: `_relaxed`, `_acquire`, `_release` for weaker/cheaper variants
  - `atomic_read`/`atomic_set` ≈ `READ_ONCE/WRITE_ONCE`
- `cmpxchg(ptr, old, new)` – fully ordered; `try_cmpxchg` returns bool and updates `old` (cleaner loops)
```c
old = READ_ONCE(*p);
do { new = f(old); } while (!try_cmpxchg(p, &old, new));
```
- `refcount_t`: saturating (no overflow → UAF); `refcount_inc_not_zero` for lookup-vs-free races; use instead of raw `atomic_t` refcounts

## 12. How Atomics Are Built in Hardware
- **x86**: `lock` prefix (cache line lock/ownership), `cmpxchg`, `xadd`
- **ARM64 LL/SC**: `ldxr` / `stxr` loop (exclusive monitor; spurious failure possible)
- **ARM64 LSE (ARMv8.1)**: single-instruction `CAS`, `LDADD`, `SWP`, `LDSET` – far better scaling under contention (far atomics at the cache/interconnect)
- Kernel patches in LSE via alternatives at boot (`CONFIG_ARM64_LSE_ATOMICS`)
- Weak CAS (`compare_exchange_weak`) may fail spuriously on LL/SC → always in a loop

## 13. Barriers + Locks
- `spin_lock` = acquire; `spin_unlock` = release → no extra barrier needed inside
- Lock/unlock **do not** order accesses across *two different* critical sections for third parties unless same lock (`smp_mb__after_spinlock()` for the strong case)
- Wakeups: `set_current_state()` has `smp_mb`; `wake_up` pairs with it → no lost wakeup
```
waiter: set_current_state(TASK_UNINTERRUPTIBLE); if (!cond) schedule();
waker : cond = 1; wake_up(...);   // barrier inside wake_up path
```

## 14. Device / MMIO Ordering (driver topics)
- Normal memory vs **Device-nGnRE** memory (ioremap) – different ordering rules
- `readl()` : orders against *later* normal memory reads (acquire-like) and DMA buffer reads
- `writel()` : orders *earlier* normal memory writes (e.g. descriptors) before the MMIO write (release-like)
- `readl_relaxed/writel_relaxed` : no such ordering – use + explicit `dma_wmb()` when batching
- Pattern:
```c
desc->addr = ...; desc->len = ...;
dma_wmb();                    // descriptor visible to device before ownership flag
desc->flags = OWN;
wmb();  /* or writel() which implies it */
writel(tail, dev->doorbell);
```
- Posted writes: MMIO write may still be in flight → **read back** a device register to flush
- `ioremap_wc` (write-combining) → needs `wmb()`/`mmiowb`-style flushing, no read side effects

## 15. Cache Maintenance & Coherency (non-coherent DMA, JIT, self-modifying)
- ARM64 caches: separate I/D at L1; **PoU** (point of unification: I/D/TLB see same) vs **PoC** (point of coherency: all observers incl. DMA)
- `DC CVAC` clean to PoC, `DC IVAC` invalidate, `DC CIVAC` clean+invalidate, `IC IVAU` invalidate I-cache
- Code loading (modules, JIT, `mprotect` exec): D-cache clean → `DSB` → I-cache invalidate → `DSB` → `ISB`
- `DMB` = ordering; `DSB` = completion (waits for cache/TLB ops); `ISB` = flush pipeline
- Shareability domains: non-shareable, inner (cores), outer, full-system (`dmb ish` vs `osh` vs `sy`)

## 16. False Sharing & Cache-Line Design
- Two hot variables on one 64B line (128B on some ARM/Apple/POWER) → line ping-pong
- Fix: `____cacheline_aligned_in_smp`, per-CPU data, padding, separate read-mostly vs write-hot fields (`struct` reordering – `pahole`)
- Detect: `perf c2c record/report` (HITM events), `perf stat -e cache-misses`
- True sharing (one hot counter) → fix by batching / per-CPU counters / `percpu_counter`

## 17. Per-CPU Data
- `DEFINE_PER_CPU`, `this_cpu_inc()`, `per_cpu(var, cpu)`, `get_cpu_var` (disables preemption)
- Access without locks only from owning CPU **with preemption disabled** (or `this_cpu_*` ops that are preempt-safe)
- Remote access needs sync; `percpu_counter` = per-CPU approx + global exact on demand

## 18. C/C++ Memory Model (userspace counterpart – see Ch. 22)
- `memory_order_relaxed / acquire / release / acq_rel / seq_cst` (`consume` effectively promoted to acquire)
- Default `seq_cst` = total order, safest, costs a full fence on stores for x86 (`xchg`) and `ldar/stlr` on ARM64
- Data race = UB; `volatile` ≠ atomic in C/C++ (only kernel conventions tolerate it)

## 19. Common Bugs
- Missing `READ_ONCE` → compiler hoists load out of loop (spin forever)
- Flag + data without release/acquire → works on x86, fails on ARM64
- Using `smp_wmb` when load side also needs `smp_rmb` (barriers must **pair**)
- Assuming `atomic_inc` orders surrounding accesses
- Dekker-style handshake with only acquire/release (needs full barrier – StoreLoad)
- MMIO: using `*ptr = val` instead of `writel`

---

## Senior Interview Questions

**Q1. What's the difference between atomicity, visibility and ordering?**
- Atomic = indivisible. Visibility = when other CPUs see it. Ordering = relative order of different locations.

**Q2. Why can this fail on ARM64 but not x86?** (`data=1; flag=1;` / `while(!flag); use(data)`)
- ARM may reorder the two stores/loads; need `smp_store_release`/`smp_load_acquire` (or `smp_wmb`/`smp_rmb` pair).

**Q3. `smp_mb` vs `mb`?** 
- `smp_*` only orders among CPUs (nop on UP); `mb` also orders against devices/MMIO.

**Q4. What do acquire and release mean? Why cheaper than full barrier?**
- One-directional fences tied to a specific access; allow reordering in the other direction.

**Q5. Do all atomic ops imply a barrier?**
- No. Only value-returning ones are fully ordered; `atomic_inc()` is not.

**Q6. What does `READ_ONCE` give you?**
- Compiler-level: single, untorn, non-fused, non-reloaded access. No CPU barrier.

**Q7. LL/SC vs LSE? Why does it matter?**
- LL/SC can livelock/scale poorly under high contention; LSE single-instruction atomics scale better.

**Q8. What is a data dependency and why does RCU rely on it?**
- Loading `p->x` after loading `p` is ordered by hardware (except Alpha, historically) → `rcu_dereference` doesn't need a read barrier.

**Q9. What's cache coherency vs memory consistency?**
- Coherence: per-location agreement. Consistency: cross-location ordering.

**Q10. Explain `dma_wmb()` vs `wmb()` vs `writel()`.**
- `dma_wmb`: lightweight, orders stores to coherent DMA memory. `wmb`: all stores incl. device. `writel`: includes the barrier needed so earlier normal-memory writes (descriptors) are seen before the MMIO write.

**Q11. How do you find false sharing?**
- `perf c2c`; look at HITM and cache-line offsets; `pahole` for struct layout.

**Q12. Why does Dekker's algorithm need a full barrier?**
- StoreLoad reorder: each CPU's store sits in its buffer while it reads other's flag as 0.

---

## Whiteboard Drill
- Draw 2 cores with store buffers + L1 + shared bus; show how SB test yields r1=r2=0
- Write SPSC ring buffer with `smp_store_release` / `smp_load_acquire`
- Translate lock acquire/release into ARM64 `ldaxr/stlxr` or `cas`

## Quick Revision
```
Compiler : READ_ONCE / WRITE_ONCE / barrier()
CPU      : smp_mb / smp_rmb / smp_wmb / acquire / release
Atomics  : returning = full barrier, non-returning = none
Device   : readl/writel (+ dma_wmb / mb); read-back to flush posted writes
x86=TSO (only StoreLoad), ARM64=weak (everything except dependencies)
Cache    : coherency per-address; consistency across addresses
```
