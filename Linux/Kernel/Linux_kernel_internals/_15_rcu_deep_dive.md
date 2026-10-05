# Chapter 15 – RCU Deep Dive

> Extends `7_synchronization.md` §42-46. Priority: ★★★★★ for Intel / AMD / Qualcomm / Broadcom kernel roles.

---

## 1. Why RCU Exists
- Read-mostly data (routing table, dentry cache, task list, module list, fd table, netdev list)
- `rwlock` readers still bounce the lock's cache line → readers do NOT scale
- RCU readers write **nothing shared** → zero contention, scales linearly with CPUs
- Cost moved to the **writer**: must wait (or defer) before freeing old data

## 2. Core Idea (3 phases)
```
1. Remove / publish   : writer swaps pointer to new version
2. Wait               : wait for a GRACE PERIOD (all pre-existing readers done)
3. Reclaim            : free the old version
```
- Readers see **either old or new** version — never a half-updated one
- Old readers may still use old object *after* the update → that is legal and the whole point

## 3. Reader API
```c
rcu_read_lock();
p = rcu_dereference(gp);      // load pointer
if (p) use(p->field);         // valid only until rcu_read_unlock()
rcu_read_unlock();
```
- `rcu_read_lock/unlock` = mark read-side critical section (RSCS)
- Non-preemptible RCU (`!CONFIG_PREEMPT_RCU`): it is just `preempt_disable()` → **no sleeping**
- `CONFIG_PREEMPT_RCU`: readers can be preempted (not sleep voluntarily) – needs extra tracking
- Cannot hold the pointer after `rcu_read_unlock()` – unless you took a refcount (`kref_get_unless_zero`) inside the RSCS
- Cannot block / `schedule()` / `mutex_lock()` inside (use **SRCU** if you must sleep)

## 4. Writer API
```c
spin_lock(&lock);                 // writers still need mutual exclusion among themselves!
old = rcu_dereference_protected(gp, lockdep_is_held(&lock));
rcu_assign_pointer(gp, new);      // publish
spin_unlock(&lock);
synchronize_rcu();                // wait for grace period (blocks)
kfree(old);
```
- Async variant: `call_rcu(&old->rcu_head, free_fn)` – callback runs after GP
- `kfree_rcu(old, rcu_head)` – batched free, no custom callback
- `rcu_barrier()` – wait for all **already queued** callbacks to finish (needed in module exit!)

## 5. Pointer Primitives – What They Really Do
- `rcu_assign_pointer()` = `smp_store_release()` → init of new object is visible **before** pointer is
- `rcu_dereference()` = `READ_ONCE()` + **address-dependency ordering** (+ lockdep/sparse checks)
  - Dependent loads (`p->field`) are ordered after loading `p` on all archs (even Alpha, historically)
  - Compiler must not re-load / speculate the pointer → that is why plain `p = gp;` is a bug
- `rcu_dereference_protected()` – for update side with lock held (no ordering needed)
- `rcu_access_pointer()` – only compare/NULL check, never dereference
- `__rcu` annotation + sparse catches missing accessors

## 6. RCU-Protected Lists
- `list_add_rcu`, `list_del_rcu`, `list_replace_rcu`, `list_for_each_entry_rcu`
- `hlist_add_head_rcu`, `hlist_del_rcu`, `hlist_for_each_entry_rcu` (hash tables)
- `list_del_rcu` leaves `next` valid (poison only `prev`) so in-flight readers can walk on
- Never use `list_del()` + immediate free in RCU lists

## 7. Grace Period (GP) and Quiescent State (QS)
- **GP** = interval after which every CPU has passed through ≥1 **quiescent state**
- **QS** (CPU is provably not inside a RSCS):
  - context switch (non-preemptible RCU)
  - idle loop / dyntick-idle
  - return to / running in userspace (matters for `nohz_full`)
  - for PREEMPT_RCU: no task on the CPU is within a RSCS (tracked via `rcu_read_lock_nesting`, blocked-task lists)
- Why it works: a reader that began **before** the writer's update must finish before GP ends; readers beginning after see new pointer

```
Reader1 ====[RSCS]=====
Writer     --update--|<------- GP ------->|--free
Reader2                    ===[RSCS]===   (sees new ptr, may overlap GP, fine)
```

## 8. Tree RCU (the real implementation)
- Problem: global QS tracking would serialise all CPUs on one lock
- Solution: **hierarchy of `struct rcu_node`**; leaf covers ≤16 CPUs (`RCU_FANOUT_LEAF`), upper fanout 64
- Each CPU reports QS to its leaf; last reporter propagates up; root completion = GP end
- Per-CPU `struct rcu_data` holds the CPU's callback list (segmented: DONE / WAIT / NEXT_READY / NEXT)
- **GP kthread** (`rcu_gp_kthread`, e.g. `rcu_preempt`) drives GP start, force-QS scans, GP end
- Callback invocation: RCU softirq (`RCU_SOFTIRQ`) or per-CPU `rcuc` kthreads (RT) or `rcuo*` kthreads (nocb)
- GP typically takes **ms to tens of ms** → `synchronize_rcu()` is slow; hot paths use `call_rcu`

## 9. Expedited GPs
- `synchronize_rcu_expedited()` – IPIs CPUs to force QS quickly (~sub-ms)
- Costs: disturbs all CPUs → bad for RT / isolated CPUs. Used e.g. in some hotplug / `memcg` paths
- `rcupdate.rcu_expedited=1` boot param; `rcu_normal` forbids it

## 10. Callback Offloading (NOCB)
- `rcu_nocbs=<cpulist>` → callbacks moved to `rcuo` kthreads, which can run on housekeeping CPUs
- Needed with `nohz_full` / `isolcpus` for latency-sensitive CPUs (no RCU softirq noise)

## 11. RCU Flavors (modern kernels merged most)
- **RCU** (vanilla; since 5.0 unified bh/sched/preempt flavors)
- **SRCU** – sleepable readers: `srcu_read_lock(&ss)` returns idx; `srcu_read_unlock(&ss, idx)`; `synchronize_srcu(&ss)`; per-domain so a slow reader only blocks *its* domain (used by notifiers, KVM, `fsnotify`)
- **Tasks RCU / Tasks Trace RCU / Tasks Rude RCU** – wait for tasks to voluntarily context switch; used by ftrace/BPF trampolines, kprobes
- **Userspace**: `liburcu` (memb, QSBR, bullet-proof flavours)

## 12. Classic Update Patterns
| Pattern | How |
|---|---|
| Replace | alloc new copy → update copy → `rcu_assign_pointer` → `call_rcu(old)` |
| Delete | unlink under lock → `synchronize_rcu`/`call_rcu` → free |
| Insert | init node fully → `list_add_rcu` (release ordering publishes init) |
| Lookup+use | `rcu_read_lock` → find → `refcount_inc_not_zero` → `rcu_read_unlock` → use w/ ref |
- Writer-side deletion must be **two-phase**: *remove from reachability*, then *wait*, then *free*

## 13. SLAB_TYPESAFE_BY_RCU
- Slab memory is not returned to page allocator until GP, but **objects can be freed and reused inside the same cache immediately**
- Reader may see object recycled as a *different instance of the same type* → must **revalidate** (e.g. recheck key / refcount) after taking ref
- Used by `struct task_struct`, `anon_vma`, `files_struct`-type lookups

## 14. RCU vs Alternatives
| Mechanism | Reader cost | Writer cost | Readers see stale? | Notes |
|---|---|---|---|---|
| rwlock/rwsem | atomic on shared line | blocks readers | no | readers bounce cacheline |
| seqlock | none, but **retry** | writer exclusive | no (retry) | pointers unsafe (freed memory) |
| RCU | ~zero | defer/wait GP | **yes, briefly** | pointers safe, memory deferred |
| refcount | atomic inc/dec | easy | no | contention on hot objects |
| hazard pointers | per-reader publish | scan | yes | newer in-kernel (6.x work) |
- RCU is *not* a lock; it is a **deferred reclamation + publication** mechanism

## 15. Memory-Ordering View
- `rcu_assign_pointer` (release) + `rcu_dereference` (dependency) = message-passing pattern
- GP guarantee = full-barrier-like: anything before `synchronize_rcu()` is ordered before anything after it relative to readers
- Do **not** rely on RCU for ordering between two independent pointers

## 16. Typical Kernel Users (name-drop in interviews)
- VFS **dcache RCU-walk** (path lookup without locks; falls back to ref-walk on failure)
- Routing / FIB, netdev list, netfilter hooks, socket lookup (`SO_REUSEPORT` groups)
- PID hash / task list (`for_each_process` under `rcu_read_lock`)
- Module list, notifier chains (`srcu_notifier`), `cpufreq`, `clk` consumers
- Page-table freeing (`mmu_gather` with RCU on some archs), `anon_vma`

## 17. RCU Stalls
- Message: `rcu: INFO: rcu_sched self-detected stall on CPU` / `detected stalls on CPUs/tasks`
- Default `CONFIG_RCU_CPU_STALL_TIMEOUT` ≈ **21 s**
- Root causes:
  - CPU spinning with preemption/IRQs disabled (long loop, spinlock hold, IRQ storm)
  - Reader that never exits RSCS (bug: sleeps, infinite loop in `rcu_read_lock`)
  - GP kthread starved (RT task hogging CPU, wrong priority) → message names `rcu_preempt kthread starved`
  - Hypervisor vCPU preempted for long
  - Offline / hot-unplugged CPU not reporting QS
- Triage: read stalled-CPU backtrace in dmesg, `ftrace` `rcu:*` events, `rcutree.rcu_cpu_stall_*` params, check IRQ storms (`/proc/interrupts`), check RT throttling
- Memory side effect: if GPs stall, **callbacks pile up → memory pressure/OOM**

## 18. Common RCU Bugs
- Dereferencing `gp` without `rcu_dereference`
- Sleeping inside `rcu_read_lock` (caught by `CONFIG_DEBUG_ATOMIC_SLEEP`, lockdep RCU checks)
- Using object after `rcu_read_unlock` without a reference
- Forgetting writer-side lock (two writers racing → lost update)
- Module unload with pending `call_rcu` callbacks → missing `rcu_barrier()` → callback jumps into freed module text
- `synchronize_rcu()` inside an RSCS → deadlock
- Calling `synchronize_rcu()` in a hot path / under a lock needed by readers

## 19. Debug / Observability
- `CONFIG_PROVE_RCU`, `CONFIG_RCU_TRACE`, `CONFIG_RCU_STRICT_GRACE_PERIOD` (stress)
- `rcutorture` (kernel self-test module), `/sys/kernel/debug/rcu/`
- `rcu_read_lock_held()`, `rcu_read_lock_bh_held()` in `WARN_ON/RCU_LOCKDEP_WARN`
- Tracepoints: `rcu_utilization`, `rcu_grace_period`, `rcu_callback`, `rcu_invoke_callback`

---

## Senior Interview Questions

**Q1. Explain RCU in 60 seconds.**
- Readers: lock-free, no shared writes. Writer: copy, update, publish with release, wait GP, reclaim.
- Safe because GP ends only after all pre-existing readers finished.

**Q2. What is a grace period / quiescent state?**
- GP: time until every CPU has had a QS. QS: CPU known not to be in an RSCS (ctx switch, idle, user mode).

**Q3. Why `rcu_dereference` and not a plain load?**
- Plain load can be torn/re-fetched by compiler, and dependency ordering isn't guaranteed to the compiler. `rcu_dereference` = `READ_ONCE` + dependency + lockdep checks.

**Q4. `synchronize_rcu` vs `call_rcu`?**
- Sync: blocks (can sleep) until GP. Call: async callback after GP; usable in atomic ctx; needs embedded `rcu_head`.

**Q5. Can RCU readers sleep?**
- No (classic). Use SRCU. PREEMPT_RCU readers can be preempted but not block.

**Q6. Why is RCU better than rwlock for read-heavy?**
- No atomic on shared line → no cacheline bouncing, readers never wait for writers.

**Q7. What's the downside?**
- Readers may see stale data; writers are slower/complex; memory freed late; GP latency; needs careful object lifetime design.

**Q8. How do you remove an element from an RCU hlist safely?**
- `spin_lock; hlist_del_rcu(&e->node); spin_unlock; call_rcu(&e->rcu, free_e)` (or `kfree_rcu`).

**Q9. How does Tree RCU scale to 1000s of CPUs?**
- Hierarchical `rcu_node` combining tree; per-CPU callback lists; QS reported to leaf, propagated by last reporter.

**Q10. Why `rcu_barrier()` in module exit?**
- Pending `call_rcu` callbacks reference module code/data; must complete before unload.

**Q11. Scenario – "system gets RCU stall every few days; how do you debug?"**
- Get stall dump → which CPU & stack → what holds preemption/IRQs off → IRQ storm? RT hog starving `rcu_gp` kthread? vCPU steal? → ftrace/`perf` on that CPU → fix (add `cond_resched`, shrink critical section, adjust RT prio/throttling).

**Q12. How does RCU interact with `nohz_full` CPUs?**
- User-mode execution counts as QS (context tracking); callbacks offloaded with `rcu_nocbs`; avoid expedited GPs.

---

## Whiteboard Drill
- Draw: 3 CPUs, a pointer `gp`, old object A, new object B, timeline of reader1/reader2/writer, mark GP boundary
- Write: RCU-protected linked-list insert, delete, lookup with refcount upgrade
- Explain: why `list_del_rcu` doesn't poison `next`

## Quick Revision
```
Reader  : rcu_read_lock / rcu_dereference / rcu_read_unlock (no sleep)
Writer  : lock -> rcu_assign_pointer -> unlock -> synchronize_rcu/call_rcu -> free
GP      : all CPUs passed a quiescent state
Tree RCU: rcu_node hierarchy + GP kthread + per-CPU callbacks
SRCU    : sleepable readers, per-domain
Stall   : CPU/reader/GP-kthread stuck ~21 s
Module  : rcu_barrier() before unload
```
