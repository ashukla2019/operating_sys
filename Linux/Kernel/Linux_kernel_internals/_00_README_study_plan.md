# Linux Internals Notes – Index & Study Plan (updated Oct 2026)

Target: senior/staff OS-internals & multithreading rounds – Qualcomm, AMD, Intel, ARM, Broadcom, HPE, NVIDIA.

## Chapters
| # | File | Focus |
|---|---|---|
| 1-14 | original chapters | breadth + interview framing |
| 15 | `_15_rcu_deep_dive.md` | **NEW** RCU API, grace periods, Tree RCU, SRCU, stalls |
| 16 | `_16_memory_ordering_atomics.md` | **NEW** barriers, acquire/release, x86 vs ARM64, MESI, MMIO ordering |
| 17 | `_17_locking_internals.md` | **NEW** qspinlock, mutex, rwsem, seqlock, futex, PREEMPT_RT, lockdep |
| 18 | `_18_numa_deep_dive.md` | **NEW** policies, balancing, tiering, device locality, diagnosis |
| 19 | `_19_mm_advanced.md` | **NEW** zones/watermarks, reclaim, THP, TLB, GUP, memcg, OOM |
| 20 | `_20_scheduler_internals.md` | **NEW** EEVDF, PELT, sched_domains, EAS, RT/DL, isolation |
| 21 | `_21_dma_iommu_coherency.md` | **NEW** DMA API, SMMU/IOMMU, PCIe, VFIO, dma-buf |
| 22 | `_22_userspace_multithreading.md` | **NEW** pthreads, C++ atomics, lock-free, thread pools, coding drills |
| 23 | `_23_debugging_hangs_crashes.md` | **NEW** oops/lockups/kdump/crash/ftrace/perf/eBPF |
| 24 | `_24_arm64_platform_internals.md` | **NEW** EL0-3, GIC, SMMU, PSCI, DT/ACPI, x86 comparison |

## Priority order (given 2-3 weeks, ~20 h/week)
1. Week 1: Ch. 16 (ordering) → 15 (RCU) → 17 (locking) → 22 (userspace MT + coding drills)
2. Week 2: Ch. 18 (NUMA) → 19 (MM) → 20 (scheduler) → 21 (DMA/IOMMU) 
3. Week 3: Ch. 24 (ARM64/x86) → 23 (debugging scenarios) → revisit original ch. 7, 8, 12 + mock interviews

## How to use each new chapter
- Read once, then close the file and explain the "Senior Interview Questions" aloud
- Re-draw the "Whiteboard Drill" diagrams from memory
- Write the code in Ch. 22 §14 from scratch (no copy/paste)

## Verify before the interview
- Kernel details change (EEVDF 6.6, per-VMA locks 6.4, PREEMPT_RT mainline 6.12, sched_ext 6.12, MGLRU 6.1). Check `Documentation/` and LWN for the kernel version the team uses.
- Practice with real source: `kernel/rcu/tree.c`, `kernel/locking/qspinlock.c`, `kernel/sched/fair.c`, `mm/page_alloc.c`, `mm/mempolicy.c`, `kernel/dma/`.
