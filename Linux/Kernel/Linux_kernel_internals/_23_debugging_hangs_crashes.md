# Chapter 23 – Kernel Debugging: Hangs, Crashes, Tracing & Profiling

> Extends `7_synchronization.md` §65-67 and `_14_kernel_modules.md` §59. Priority: ★★★★★ (senior rounds always include "how would you debug X?")

---

## 1. General Method (say this first in any debug question)
1. **Define symptom** (hang / crash / corruption / latency / leak / perf drop) & frequency
2. **Collect evidence**: logs, dumps, traces, versions, recent changes, HW/FW
3. **Narrow scope**: reproducible? which CPU/context/subsystem? bisect (config, patch, hardware)
4. **Hypothesis → test**: add tracing, assertions, sanitizers (non-invasive first)
5. **Fix → verify under stress → add regression test / assertion**

## 2. Reading an Oops / Panic
```
BUG: kernel NULL pointer dereference, address: 0000000000000008
#PF: supervisor read access in kernel mode
RIP: 0010:foo_read+0x2a/0x90 [mydrv]      <- (arm64: pc : foo_read+0x2c/0x90, lr : ...)
Call Trace: ... 
Code: 48 8b 47 08 ...
Tainted: G  O  (proprietary/out-of-tree)
```
- Identify: faulting address (NULL+offset → field offset), function+offset, call trace, registers, `Tainted` flags, CPU/PID/comm, kernel version
- Map to source: `addr2line -e vmlinux`, `gdb vmlinux; list *(foo_read+0x2a)`, `scripts/faddr2line`, `scripts/decodecode`, `objdump -dS`
- ARM64 extras: `ESR_EL1` decode (EC field: data abort, instruction abort, SError), `FAR_EL1` fault address, "Internal error: Oops: 96000005" (DFSC = translation fault level 1)
- Distinguish: **oops** (recoverable-ish, kills task) vs **panic** (fatal; `panic_on_oops`), **BUG()/WARN()**, **machine check / SError** (hardware)
- Taint flags: `P` proprietary, `O` out-of-tree, `F` forced module load, `W` earlier warning, `D` died (oops), `L` soft-lockup, `E` unsigned module…

## 3. Hang Categories & Detectors
| Type | Detector | Default | Meaning |
|---|---|---|---|
| **Soft lockup** | watchdog (hrtimer + `watchdog/N` kthread) | ~20 s (`2*watchdog_thresh`) | CPU stuck in kernel without scheduling (preempt off loop) |
| **Hard lockup** | NMI watchdog (perf-based; arm64 pseudo-NMI or buddy-CPU detector) | ~10 s | CPU stuck with **IRQs disabled** |
| **Hung task** | `khungtaskd` | 120 s (`kernel.hung_task_timeout_secs`) | task in **D state** (TASK_UNINTERRUPTIBLE) too long |
| **RCU stall** | RCU | ~21 s | CPU/reader/GP kthread blocking grace period |
| **Workqueue lockup** | wq watchdog | `workqueue.watchdog_thresh` | worker pool made no progress |
| **Deadlock (software)** | lockdep (potential), hung-task (actual) | – | – |
| **SysRq/NMI** | manual | – | on-demand dumps |
- Tunables: `kernel.softlockup_panic`, `kernel.hardlockup_panic`, `kernel.hung_task_panic`, `kernel.panic_on_rcu_stall`, `kernel.panic=N` (reboot after N s)

## 4. Triage a "System Hangs" Report
1. Is it **dead** (no ping, no console) or **sluggish**? Console/serial/IPMI/BMC SOL accessible?
2. Magic SysRq (if enabled `kernel.sysrq=1`): `echo w > /proc/sysrq-trigger` (blocked tasks), `l` (all CPU backtraces), `t` (all tasks), `m` (memory), `c` (crash → trigger kdump), `s/u/b` sync/remount/reboot
3. Look for common stack tops: `mutex_lock`, `rwsem_down_*` (who holds? `/proc/<pid>/stack`), `io_schedule` (IO wait), `schedule_timeout`, `do_exit`, `wait_for_completion` (driver waiting on HW), `__lock_text_start`
4. **Find the lock owner** (mutex owner shown in hung-task output with `CONFIG_DETECT_HUNG_TASK`; lockdep `Showing all locks held`)
5. CPU stuck? `perf top`/NMI backtrace of that CPU: spinning in `queued_spin_lock_slowpath` (lock contention/deadlock) vs busy loop vs IRQ storm (`/proc/interrupts` deltas)
6. HW: PCIe AER, MCE (`mcelog`/`rasdaemon`), thermal throttle, firmware, watchdog reset reason
- **High load average + idle CPU** ⇒ many D-state tasks (IO/lock) — not CPU problem

## 5. kdump / crash
- Setup: reserve memory `crashkernel=256M` (or `auto`), load capture kernel via `kexec -p` (`kdump.service`), dump to `/var/crash` (vmcore, `makedumpfile` compresses/filters)
- Triggers: panic, `sysrq-c`, NMI (`unknown_nmi_panic`, `panic_on_unrecovered_nmi`), watchdog panic
- ARM/embedded: `ramoops`/**pstore** (RAM-backed logs survive reboot), vendor **minidump** (Qualcomm), JTAG/**Trace32**, CoreSight ETM trace
- **crash utility** (needs matching `vmlinux` with debuginfo):
  - `bt`, `bt -a` (all CPUs), `ps`, `ps -m` (age in state), `log` (dmesg), `files`, `vm`, `kmem -s/-i`, `struct task_struct <addr>`, `rd`, `dis`, `mod -s`, `foreach bt`, `waitq`, `irq`, `net`, `dev`, `sys`
  - Find lock owner: print `struct mutex` → `owner` → `task_struct` → `bt`
  - Find deadlock: `foreach UN bt` (all uninterruptible tasks), compare held lock chains
- Tips: check `lockdep` data in dump, `percpu` vars (`p var:a`), per-CPU runqueue (`runq`)

## 6. Dynamic Tracing Toolbox
**ftrace (`/sys/kernel/tracing`)**
- Tracers: `function`, `function_graph` (call tree + durations), `irqsoff`, `preemptoff`, `wakeup`, `wakeup_rt`, `hwlat`, `osnoise`, `timerlat`
- `set_ftrace_filter`, `set_graph_function`, `trace_options`, events under `events/` (`sched`, `irq`, `block`, `rcu`, `lock`, `kmem`…)
- Front-ends: `trace-cmd record/report`, KernelShark, `perf trace`
- `trace_printk()` (debug), `tracepoint`s (stable-ish), `ftrace_dump_on_oops`, `traceoff_on_warning`
**kprobes/kretprobes/uprobes/fprobes**: dynamic probes without recompiling (`echo 'p:myprobe foo_read arg1=%di' > kprobe_events`)
**perf**: `stat`, `record -g`, `report`, `top`, `sched`, `lock`, `c2c`, `mem`, `trace`, `annotate`, PMU events (cycles, cache-misses, branch-misses, `mem-loads`), flame graphs
**eBPF** (bcc/bpftrace/libbpf/CO-RE): `runqlat`, `offcputime`, `funclatency`, `biolatency`, `tcpconnect`, `opensnoop`, `profile`; custom `bpftrace -e 'kprobe:vfs_read { @[comm]=count(); }'`
**Others**: `strace -f -tt -T`, `ltrace`, `gdb`/`rr`, `/proc` & `/sys`, `dynamic_debug` (`echo 'file drv.c +p' > /sys/kernel/debug/dynamic_debug/control`), `printk` ratelimits

## 7. Memory/Concurrency Bug Detectors
| Tool | Finds |
|---|---|
| **KASAN** (generic/SW-tags/HW-tags on arm64 MTE) | UAF, OOB on heap/stack/globals |
| **KFENCE** | low-overhead sampling UAF/OOB in production |
| **KMSAN** | uninitialised reads |
| **UBSAN** | undefined behaviour (overflow, shift, bounds) |
| **KCSAN** | data races (sampled watchpoints) |
| **lockdep** | lock-order inversion, IRQ-safety violations |
| **kmemleak** | leaks (scans for unreferenced allocations) |
| **SLUB debug** (`slub_debug=FZPU`) | redzones, poisoning, user tracking |
| **DEBUG_ATOMIC_SLEEP** | sleeping in atomic context |
| **DEBUG_PAGEALLOC / PAGE_POISONING / page_owner** | page misuse, who allocated page |
| **debugobjects** | objects used before init / after free |
| **FAULT_INJECTION** | force allocation / IO failures for error-path testing |
| **syzkaller** | fuzzing |
- Usage: debug builds in CI/stress; combine KASAN + lockdep + KCSAN on separate runs (overhead)

## 8. Latency/Jitter Debug
- **Where does time go?** off-CPU (blocked) vs on-CPU (running) vs runnable-wait
- `perf sched latency/timehist`, `runqlat`, `offcputime`, `irqsoff`/`preemptoff` tracers, `osnoise` for isolated cores, `hwlat` for SMI
- Common sources: IRQ/softirq storms, long non-preemptible sections, THP compaction, CPU frequency/idle states (C-state exit latency), RT throttling, SMIs, cgroup throttling, lock contention, page faults, NUMA remote access

## 9. Memory Issue Debug
- Leak in kernel: `slabtop`, `/proc/meminfo` (Slab, SUnreclaim, VmallocUsed, PageTables), `/proc/slabinfo`, `kmemleak`, `page_owner`
- Leak in user: `smaps_rollup`, `pmap -x`, valgrind/ASAN/heaptrack, `malloc_stats`
- Fragmentation: `/proc/buddyinfo`, `/proc/pagetypeinfo`, `/proc/vmstat` (compaction_*)
- OOM analysis: read the OOM dump (per-zone free, per-order, process table with `oom_score_adj`), PSI

## 10. Embedded / SoC Debug Specifics (Qualcomm/ARM/Broadcom)
- **Early boot**: `earlycon`, `console=ttyS0,115200`, `loglevel=8`, `initcall_debug`, `ignore_loglevel`, `printk.time`
- **Bus errors / Synchronous External Aborts**: access to unclocked/powered-off peripheral – check clocks, regulators, power domains (genpd), pinctrl, reset lines, device tree status, and probe order
- **Probe deferral**: `-EPROBE_DEFER`, `/sys/kernel/debug/devices_deferred`
- **Device tree issues**: `dtc -I fs /proc/device-tree`, overlays, wrong `reg`/`interrupts` cells
- **Hardware debug**: JTAG (OpenOCD/Trace32), CoreSight ETM/STM trace, logic analyzer/scope, UART logs, PMIC fault registers, watchdog reset cause
- **Secure world** crashes (TrustZone/TEE) look like unexplained resets → check secure logs, firmware crash dumps (minidump)

## 11. Debug Scenarios (practice answers)
| Scenario | Answer outline |
|---|---|
| Random kernel panic in driver once a week | collect vmcore, decode oops, check taint, enable KASAN/slub_debug/lockdep in test, review lifetime/refcount/concurrency at remove/unbind/IRQ path |
| Deadlock between two drivers | hung-task output, `foreach UN bt`, lock owners, lockdep splat, lock-order graph, fix ordering / reduce lock scope |
| Device works, then DMA corrupts memory | IOMMU fault logs, `dma-debug`, check unmap/reset ordering, bounds, stale descriptors, endianness |
| Interrupt storm | `/proc/interrupts` rate, `irq/N` threaded handler CPU use, check level vs edge, unacked source, `irqpoll`, `nobody cared` message (`IRQ N: nobody cared`) |
| High CPU in `ksoftirqd` | NAPI budget, softirq overload, `/proc/softirqs`, RPS/RFS, IRQ affinity, offload |
| Latency spike every N seconds | periodic timers/kworker/THP/khugepaged/cron/SMI → `ftrace`/`osnoise`, correlate with `perf sched timehist` |
| Boot hang after "Starting kernel" | earlycon, initcall_debug, memory map/DT, bootargs, missing console, bad clocks |
| Memory grows steadily | slab vs anon vs page cache split, kmemleak, `page_owner`, cgroup stats |

---

## Senior Interview Questions

**Q1. Soft lockup vs hard lockup vs hung task vs RCU stall?**
- Soft: CPU in kernel not scheduling; hard: IRQs disabled; hung task: D-state too long; RCU stall: GP blocked.

**Q2. You get a kernel panic on a customer machine. What do you ask for?**
- Console/oops text, vmcore + vmlinux with debuginfo, kernel/module versions, taint, recent changes, HW info, reproduction steps.

**Q3. How do you find who holds a mutex in a vmcore?**
- `struct mutex` → `owner` (strip flag bits) → `task_struct` → `bt`.

**Q4. When would you use ftrace vs perf vs eBPF?**
- ftrace: function/event timing, low-overhead built-in; perf: sampling profile & PMU; eBPF: programmable aggregation/filtering in kernel.

**Q5. What does KASAN need and cost?**
- Shadow memory, compiler instrumentation; ~2x slowdown/mem; HW-tag KASAN on ARM MTE lower cost.

**Q6. System shows load average 50, CPU idle. Why?**
- Tasks in D-state (disk/NFS/lock); look at `ps -eo stat,wchan`, blocked-task dump.

**Q7. How do you debug a bus error in a new SoC peripheral driver?**
- Check clock/regulator/power-domain/reset enablement before register access, DT `reg`, probe order, secure ownership, and decode ESR/FAR.

**Q8. Difference between WARN_ON, BUG_ON, panic?**
- WARN: log+continue; BUG: oops (kill context; may panic); panic: halt/reboot. Prefer WARN + graceful error.

**Q9. How do you capture a transient issue in production with minimal overhead?**
- ftrace with `traceoff_on_warning`/trigger, ring-buffer `snapshot`, BPF probes with ratelimit, KFENCE, kdump on trigger.

**Q10. `IRQ N: nobody cared` — what happened?**
- IRQ handler(s) returned IRQ_NONE repeatedly → kernel disables the line; source not acked, wrong shared-IRQ handling, spurious/level mismatch.

---

## Quick Revision
```
Oops       : fault addr + RIP/PC + call trace + taint -> addr2line/gdb list *fn+off
Hang types : soft (~20s) / hard (NMI) / hung task (120s D-state) / RCU stall (~21s)
Dumps      : kdump + crash (bt, ps, foreach UN bt, struct) ; pstore/minidump on SoC
Tracing    : ftrace, kprobes, perf, bpftrace ; osnoise/timerlat for jitter
Detectors  : KASAN KFENCE KCSAN UBSAN lockdep kmemleak DEBUG_ATOMIC_SLEEP
```
