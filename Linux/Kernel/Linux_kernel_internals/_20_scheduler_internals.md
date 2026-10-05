# Chapter 20 – Scheduler Internals (CFS → EEVDF, PELT, Load Balancing, RT/DL, Isolation)

> Extends `_10_scheduling.md`. Priority: ★★★★☆ (★★★★★ for Qualcomm – EAS/big.LITTLE)

---

## 1. Scheduling Classes (priority order)
```
stop_sched  >  dl_sched (SCHED_DEADLINE)  >  rt_sched (FIFO/RR)  >  fair_sched (NORMAL/BATCH/IDLE)  >  idle_sched
                                                     (+ ext_sched / sched_ext: BPF-defined scheduler, 6.12+, sits at fair level)
```
- `pick_next_task()` iterates classes high→low
- `struct sched_class` has callbacks: `enqueue_task`, `dequeue_task`, `pick_next_task`, `put_prev_task`, `task_tick`, `select_task_rq`, `check_preempt_curr`…

## 2. Per-CPU Runqueue
- `struct rq` (one per CPU) with `cfs_rq`, `rt_rq`, `dl_rq`, `curr`, `idle`, `clock`, `nr_running`, `rq->lock` (raw spinlock)
- Tasks are queued on exactly one rq; migration locks **two** rq's (ordered by CPU number → `double_rq_lock`)
- `rq->lock` is the hottest lock in the system; scheduler avoids holding it long

## 3. CFS (Completely Fair Scheduler) – classic
- Each task has **`vruntime`** = runtime scaled by weight: `vruntime += delta_exec * NICE_0_WEIGHT / weight`
- Runnable tasks in an **rbtree** keyed by vruntime; pick **leftmost** (smallest vruntime)
- **Weight table**: nice 0 = 1024; each nice step ≈ ×1.25 (nice −1 ≈ 1277; nice +1 ≈ 820) → 10% CPU difference per step
- New task starts at `min_vruntime` (+ penalty); waking sleeper placed near `min_vruntime` (bounded credit) so it can preempt but not monopolise
- Timeslice derived from `sched_latency` / `nr_running` bounded by min granularity (old design)

## 4. EEVDF (Earliest Eligible Virtual Deadline First) – default since 6.6
- Replaces CFS pick-leftmost heuristics with a provable fairness model
- Concepts:
  - **Virtual time V**: weighted average of vruntimes
  - **Lag** = ideal service − actual service. Eligible if lag ≥ 0 (task not over-served)
  - **Virtual deadline** = eligible time + request/weight (request = slice)
- Pick: among **eligible** tasks, the one with **earliest virtual deadline**
- Short slice ⇒ earlier deadline ⇒ lower latency (without more CPU share) → latency control via slice (`sched_attr.sched_runtime`, "latency-nice" ideas)
- Removes many wakeup-preemption and sleeper-fairness hacks
- Interview line: "CFS = fairness by smallest vruntime; EEVDF = fairness + explicit latency via deadlines"

## 5. PELT – Per-Entity Load Tracking
- Tracks each entity's (task, cfs_rq, group) **load_avg**, **runnable_avg**, **util_avg** using **geometric decay**: contribution halves every ~32 ms (y^32 = 0.5, per-1024µs periods)
- `util_avg` ∈ [0, 1024] ≈ CPU capacity fraction used
- Used by: load balancer, **schedutil** cpufreq governor, **EAS** placement, `uclamp`, NUMA/placement heuristics
- Blocked load of sleeping tasks decays on the rq → ongoing "remembered" load

## 6. Wakeup Path
```
wake_up() -> try_to_wake_up(p):
   p->state = RUNNING after memory barrier handshake with set_current_state
   cpu = select_task_rq(p)            // class hook: wake_affine / select_idle_sibling
   if cpu != this_cpu and wakelist:  queue on remote CPU's wake list + IPI (ttwu_queue_wakelist)
   else: rq lock; enqueue_task; check_preempt_curr (set TIF_NEED_RESCHED / resched_curr)
```
- `p->on_cpu` handshake prevents wake-before-fully-switched-out race
- **wake_affine**: pull wakee to waker's CPU/LLC if they share data (cache-hot) – may hurt NUMA/balance
- **select_idle_sibling**: scan idle CPU/core in the LLC (SIS_UTIL limits scan)

## 7. schedule() and Context Switch
```
schedule() -> __schedule(): preempt_disable; rq_lock; deactivate prev if sleeping;
   next = pick_next_task(); if (prev != next) context_switch(rq, prev, next):
        switch_mm_irqs_off()   // page table + ASID/PCID (user->user); lazy TLB for kthreads
        switch_to()            // save/restore callee-saved regs, SP, (FPU lazily), thread-local (TPIDR/FS/GS)
        finish_task_switch()   // runs in NEXT task's context: release rq lock, drop prev->mm ref, put_task_struct if dead
```
- `TIF_NEED_RESCHED` flag checked on return from IRQ/syscall and at `preempt_enable()`
- **Preemption points**: voluntary (`cond_resched`, `might_sleep`), involuntary (return from interrupt to user; return to kernel when preemptible)
- Models: `none`, `voluntary`, `full`, `RT`, lazy (newer)

## 8. Scheduler Tick & NOHZ
- Periodic tick (`HZ` 100/250/1000): `scheduler_tick()` → `task_tick` (update vruntime, check slice expiry), PELT update, trigger balance
- **NOHZ idle**: stop tick on idle CPUs; one CPU does idle load balancing for others (nohz balancer)
- **NOHZ_FULL**: stop tick on busy CPUs with a single runnable task → lower jitter (needs `isolcpus`/`rcu_nocbs`/`irqaffinity`)
- hrtimers drive preemption timing in some configs

## 9. Load Balancing & sched_domains
- Hierarchy mirrors hardware: **SMT → cluster/MC (LLC) → PKG/DIE → NUMA** (`struct sched_domain` + `sched_group`)
- Balancing types:
  - **Periodic** (`rebalance_domains` in `SCHED_SOFTIRQ`, interval grows with domain level)
  - **Newidle** (CPU about to idle pulls work)
  - **Idle/nohz** balancing
  - **Wakeup** placement
  - **Active balance** (migration thread `stopper` moves running task)
- Metrics: group load/util/capacity, imbalance type (`migrate_load`, `migrate_util`, `migrate_task`, `misfit`)
- Costs: cache-cold migration, NUMA penalty → thresholds `imbalance_pct`, `cache_hot` checks, `nr_balance_failed`
- Debug: `/sys/kernel/debug/sched/domains`, `schedstat`

## 10. Asymmetric CPUs & EAS (Qualcomm / Arm big.LITTLE / DynamIQ)
- CPUs have different **capacity** (`cpu_capacity`, DT `capacity-dmips-mhz`)
- **Misfit tasks**: task util exceeds little-core capacity → upmigrate to big
- **EAS (Energy Aware Scheduling)**: for wakeups when system not overutilised, pick CPU minimising **estimated energy** using **Energy Model** (perf domains = cpufreq domains)
  - Needs: `schedutil`, EM registered (`dev_pm_opp` / DT `dynamic-power-coefficient`), asymmetric capacity
- **`uclamp`**: per-task/cgroup min/max utilisation clamps → hint boosting/capping (Android uses heavily)
- **schedutil**: cpufreq governor driven directly by scheduler util (`sugov`) → frequency changes on enqueue/tick
- Thermal & capacity pressure feed back into capacity

## 11. Group Scheduling (cgroups)
- `cpu.weight` (proportional share, cgroup v2; was `cpu.shares`), `cpu.max` (**CFS bandwidth**: `quota period`), `cpu.uclamp.*`, `cpu.idle`
- Hierarchical: each cgroup has per-CPU `sched_entity` + `cfs_rq`
- **CFS bandwidth throttling**: group exhausts quota → throttled until next period → causes latency spikes for bursty containers (classic K8s CPU-limit issue; `cpu.stat` `nr_throttled`)
- `cpuset` restricts CPUs/mems; RT tasks in cgroups need `RT_GROUP_SCHED` runtime allocation (rarely used)

## 12. Real-Time
- `SCHED_FIFO` (prio 1-99, runs until block/yield/higher prio), `SCHED_RR` (same prio round-robin with timeslice)
- Per-CPU **priority arrays/bitmap**; **push/pull** migration to keep highest-priority tasks running (`push_rt_tasks`, `pull_rt_task`)
- **RT throttling**: `sched_rt_runtime_us/sched_rt_period_us` default 950000/1000000 → RT class limited to 95% so system isn't locked up (can set -1 to disable – dangerous)
- RT starvation of kthreads (`rcu_gp`, `kworker`) → RCU stalls/watchdog; consider priorities for critical kthreads
- Combine with `PREEMPT_RT`, `isolcpus`, IRQ affinity, `mlockall`, avoid page faults, `clock_nanosleep(TIMER_ABSTIME)`

## 13. SCHED_DEADLINE
- Parameters: `runtime`, `deadline`, `period` (runtime ≤ deadline ≤ period)
- **EDF** (earliest absolute deadline) + **CBS** (constant bandwidth server) for isolation/throttling
- **Admission control**: Σ runtime/period ≤ capacity (per root domain) else `EBUSY`
- Highest user-visible class (above RT); GRUB reclaiming; used for periodic media/control tasks

## 14. Isolation Toolkit
| Tool | Effect |
|---|---|
| `isolcpus=`, `cpuset` isolated partitions | remove CPUs from general load balancing |
| `nohz_full=` | stop tick when 1 task |
| `rcu_nocbs=` | RCU callbacks off these CPUs |
| `irqaffinity=` / `/proc/irq/*/smp_affinity` | keep device IRQs away |
| `kthread_cpus`, workqueue `affinity` | move kworkers away (unbound WQ cpumask) |
| `tsc=reliable`, `skew_tick`, `nosoftlockup` | reduce noise sources |
- Verify with `osnoise`/`timerlat` tracers, `cyclictest`, `hwlat` (SMI noise)

## 15. Priorities Quick Table
- Kernel prio: 0-99 RT (lower number = higher prio internally), 100-139 normal (nice −20..19), DL = −1 (below 0)
- `chrt -f 50 cmd`, `nice -n 5`, `ionice` (IO scheduler separate), `taskset -c 2,3`

## 16. Observability
- `/proc/<pid>/sched`, `/proc/<pid>/schedstat`, `/proc/schedstat`, `/sys/kernel/debug/sched/`
- `perf sched record/latency/timehist/map`, `trace-cmd -e sched`, `bpftrace`/bcc: `runqlat`, `runqlen`, `cpudist`, `offcputime`, `cpuwalk`
- Metrics: runqueue latency (wait to run), voluntary vs involuntary ctx switches (`pidstat -w`), migrations, throttling (`cpu.stat`)
- PSI: `/proc/pressure/cpu`

## 17. Common Scenarios
| Scenario | Likely cause / steps |
|---|---|
| High load average, low CPU use | tasks in **D state** (IO/lock), count includes uninterruptible; check `ps -eo stat,wchan`, blocked-task dump |
| One CPU 100%, others idle | affinity pinning, single-threaded, IRQ on that CPU, RT hog; check `mpstat -P ALL`, `/proc/interrupts` |
| Latency spikes in container | CFS throttling (`nr_throttled`), noisy neighbour, CPU steal, THP compaction |
| High context switches | lock contention, too many threads, ping-pong wakeups, timers; `pidstat -w`, `perf trace -s` |
| RT task misses deadlines | other RT/IRQ preempting, throttle, page faults, SMI, cpufreq transition, shared lock w/o PI |
| Tasks bounce between CPUs | wake_affine, load balancing; pin or check imbalance |

---

## Senior Interview Questions

**Q1. How does CFS decide who runs next? How is EEVDF different?**
- CFS: smallest vruntime. EEVDF: among eligible (lag ≥ 0), earliest virtual deadline; slice encodes latency.

**Q2. What is vruntime and how do nice values affect it?**
- Weighted runtime; lower weight (higher nice) → vruntime advances faster → less CPU.

**Q3. What is PELT and who consumes it?**
- Decayed per-entity load/util signal; used by load balancer, schedutil, EAS, uclamp.

**Q4. Walk through `schedule()`.**
- Disable preempt, lock rq, handle prev state, `pick_next_task` over classes, `context_switch` (mm switch, `switch_to`), `finish_task_switch`.

**Q5. How does the kernel avoid lost wakeups between `wake_up` and sleeper?**
- `set_current_state()` before condition check with barrier; `wake_up` sets state RUNNING under pi_lock; pairing barriers; re-check condition after waking.

**Q6. Explain sched_domains and the balancing types.**
- Hardware-mirroring hierarchy; periodic, newidle, nohz idle, wakeup, active balance.

**Q7. What is EAS and when is it enabled?**
- Energy-aware wakeup placement on asymmetric systems using Energy Model + schedutil when not overutilised.

**Q8. SCHED_FIFO vs SCHED_RR vs SCHED_DEADLINE?**
- FIFO: no timeslice; RR: RR within same prio; DL: EDF+CBS with admission control, top priority.

**Q9. Why would a container with CPU limit still see latency spikes?**
- CFS bandwidth throttling when quota exhausted early in period.

**Q10. How do you build a low-jitter CPU?**
- `isolcpus`/cpuset partition, `nohz_full`, `rcu_nocbs`, IRQ affinity, RT policy, `mlockall`, avoid shared locks, verify with `osnoise`/`cyclictest`.

**Q11. What causes priority inversion and how is it handled in the scheduler?**
- Lock-holder preempted by medium task; fix with PI (`rt_mutex`), keep critical sections short, avoid sharing locks across RT/non-RT.

**Q12. `nr_running` is 1 but latency is bad. Why?**
- IRQ/softirq time, SMIs, frequency/idle-state exit latency, thermal, page faults, hypervisor steal.

---

## Quick Revision
```
Classes    : stop > deadline > rt > fair(EEVDF) > idle
Fair       : weighted vruntime/virtual deadlines, per-CPU rq, cgroup hierarchy
PELT       : decayed load/util -> balancing, schedutil, EAS, uclamp
Balancing  : sched_domains (SMT/MC/PKG/NUMA); periodic/newidle/nohz/wakeup/active
RT         : FIFO/RR (1-99) + 95% throttle; DL = EDF+CBS+admission
Isolation  : isolcpus + nohz_full + rcu_nocbs + irqaffinity
```
