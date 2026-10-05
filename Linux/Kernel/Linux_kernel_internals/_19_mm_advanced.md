# Chapter 19 – Advanced Memory Management Internals

> Extends `3_memory_mgmt.md`. Priority: ★★★★☆ (★★★★★ for Intel/AMD/Qualcomm kernel MM-adjacent roles)

---

## 1. Core Structures (modern kernels)
- `struct page` → being replaced by **`struct folio`** (a power-of-2 group of pages; head page); large folios in page cache
- `struct mm_struct` : `pgd`, `mmap_lock`, VMA tree, `mm_users` vs `mm_count`
- `struct vm_area_struct` (VMA): start/end, flags (`VM_READ/WRITE/EXEC/SHARED/IO/PFNMAP/GROWSDOWN`), `vm_ops`, `vm_file`, `anon_vma`
- **Maple tree** (6.1+) replaced rbtree+list for VMAs; **per-VMA locks** (6.4+) let page faults avoid `mmap_lock`
- `mm_users` = users of address space (threads); `mm_count` = references to the struct (lazy TLB, kernel threads)

## 2. Zones, Watermarks, Allocation Path
- Zones: `ZONE_DMA`, `ZONE_DMA32`, `ZONE_NORMAL`, `ZONE_MOVABLE`, `ZONE_DEVICE` (32-bit also HIGHMEM)
- Zone constraint comes from **device DMA masks** and migratability
- Watermarks per zone: `min` < `low` < `high`
  - free < `low` → wake `kswapd`; kswapd sleeps when free ≥ `high`
  - free < `min` → allocator does **direct reclaim** (caller stalls); `GFP_ATOMIC` can dip into reserves
- Allocation path: fast path (per-CPU page lists → buddy, `get_page_from_freelist`) → slow path (wake kswapd, compaction, direct reclaim, OOM)
- **Per-CPU page lists (pcp)** avoid zone lock for order-0 pages
- Fragmentation control: **migratetypes** (UNMOVABLE, MOVABLE, RECLAIMABLE) grouped in pageblocks

## 3. GFP Flags (frequent interview trap)
| Flag | Meaning |
|---|---|
| `GFP_KERNEL` | may sleep, may reclaim, may do IO/FS |
| `GFP_ATOMIC` | no sleep (IRQ/spinlock), uses reserves, may fail |
| `GFP_NOWAIT` | no sleep, no direct reclaim, no reserves |
| `GFP_NOIO` / `GFP_NOFS` | forbid recursion into IO / filesystem during reclaim (avoid deadlock in block/fs paths) |
| `GFP_USER` / `GFP_HIGHUSER_MOVABLE` | userspace pages |
| `__GFP_ZERO`, `__GFP_NOWARN`, `__GFP_RETRY_MAYFAIL`, `__GFP_NOFAIL`, `__GFP_COMP`, `GFP_DMA32` | modifiers |
- `memalloc_noio_save()/nofs_save()` scoped API preferred over per-call flags

## 4. Reclaim
- **LRU lists** per node/memcg: active/inactive × anon/file (+ unevictable)
- Second-chance: referenced bit → promote; unreferenced inactive → reclaim
- **MGLRU** (6.1+): multi-generation LRU, better aging via page-table scanning; `lru_gen` sysfs
- **Workingset/refault detection**: shadow entries in page cache track refault distance → protect working set
- Reclaim of: clean page cache (drop), dirty (write back first), anon (needs swap), slab (`shrinkers`), memcg limits
- `vm.swappiness` : relative cost anon-vs-file reclaim
- **Writeback**: per-bdi flusher threads; thresholds `vm.dirty_background_ratio`, `vm.dirty_ratio`, `dirty_expire_centisecs`; writers throttled by `balance_dirty_pages`
- **Compaction**: migrates movable pages to build high-order contiguity (`kcompactd`, direct compaction)
- **Swap**: swap cache, swap slots, `zswap` (compressed cache), `zram` (compressed RAM block device – common on Android/embedded)

## 5. OOM Killer
- Triggered when reclaim fails; picks by `oom_score` (RSS + swap + pagetables, adjusted by `oom_score_adj` −1000..1000)
- `-1000` = never kill; memcg OOM is scoped to the cgroup; `vm.panic_on_oom`
- Userspace: `systemd-oomd`, `lmkd` (Android – very relevant for Qualcomm), PSI (`/proc/pressure/memory`) to act before OOM
- Log: `dmesg` shows "Out of memory: Killed process …" + memory state dump (zone info, per-order free lists)

## 6. Overcommit
- `vm.overcommit_memory`: 0 heuristic, 1 always, 2 strict (`commit_limit`)
- `malloc` success ≠ memory available; failure appears at first touch (OOM) – that's why Linux "kills" instead of returning NULL

## 7. cgroup v2 memory controller (memcg)
- Files: `memory.max` (hard), `memory.high` (throttle + reclaim), `memory.low/min` (protection), `memory.swap.max`, `memory.stat`, `memory.events`, `memory.pressure`
- Charges: anon, page cache, kernel (slab, page tables, sockets…)
- Exceeding `max` → reclaim → OOM within group; containers' view in `/proc/meminfo` is host-wide (needs lxcfs etc.)

## 8. Page Fault Path (detail)
```
exception -> do_page_fault / do_mem_abort (arm64: ESR decode)
 -> find VMA (per-VMA lock or mmap_lock read)
 -> permission check -> handle_mm_fault
     -> walk/alloc page tables (pgd/p4d/pud/pmd/pte)
     -> do_anonymous_page   (zero page / new page)
     -> do_read_fault/do_fault -> filemap fault (page cache) / ->fault op
     -> do_cow_fault / do_wp_page (copy-on-write)
     -> do_swap_page        (swap in, swap cache)
     -> do_numa_page        (NUMA hinting)
 -> return / SIGSEGV / SIGBUS (e.g. beyond EOF) / OOM
```
- Kernel-mode faults on user addrs use **exception tables** (`copy_from_user` fixups)
- Major vs minor fault; `userfaultfd` lets userspace handle faults (live migration, GC)

## 9. TLB Management
- TLB caches VA→PA; flush on `munmap`, `mprotect`, page migration, COW unmap, reclaim
- **x86 shootdown**: initiator sends **IPIs** to CPUs in `mm_cpumask` → all run `flush_tlb_func` → expensive at scale (batched via `mmu_gather`)
- **ARM64**: `TLBI ... IS` **broadcast** inner-shareable instructions + `DSB ISH` → no IPI needed
- **PCID (x86) / ASID (ARM64)**: tag TLB entries by address space → no full flush on context switch
- **Lazy TLB**: kernel threads borrow previous `mm` to skip switch
- KPTI (Meltdown) doubles page tables → PCID essential for perf
- Huge pages reduce TLB misses/page-walk depth

## 10. Huge Pages
- **hugetlbfs**: explicit, reserved (`vm.nr_hugepages`, `hugepages=` boot), 2 MB/1 GB (x86), 64K/2M/32M/512M/1G… on ARM64 depending on granule (contiguous-PTE bit)
- **THP**: transparent; modes `always|madvise|never`; allocated at fault or collapsed by `khugepaged`; `defrag` knob decides stall behaviour; can cause latency spikes/compaction stalls and memory bloat
- Benefits: fewer TLB misses, shorter page walks. Risks: fragmentation, latency, NUMA-misplacement
- Databases often disable THP, use explicit hugetlb; `MAP_HUGETLB`, `madvise(MADV_HUGEPAGE)`
- mTHP / large folios (6.x) – sizes between 4K and PMD size

## 11. Pinning & GUP
- **`get_user_pages()`** (GUP) = translate user VA → struct page, take ref (used by direct I/O, RDMA, VFIO, GPU)
- **`pin_user_pages()`** (`FOLL_PIN`) – use for DMA long-term pins (distinguishes DMA pin from plain ref)
- **Long-term pins** prevent migration/compaction → must not be in `ZONE_MOVABLE`/CMA (kernel migrates pages out first: `FOLL_LONGTERM`)
- Pinned memory counts against `RLIMIT_MEMLOCK` / cgroup
- `mlock()`, `MAP_LOCKED`: keep resident but may still migrate; **pinning** forbids migration

## 12. mmu_notifier
- Lets secondary MMUs (KVM, IOMMU SVA, GPU/HMM, RDMA ODP) get callbacks when CPU page tables change (`invalidate_range_start/end`)
- Core of **SVM/HMM** – device shares CPU address space; very relevant for AMD/NVIDIA/Intel GPU & accelerator roles

## 13. Slab Allocator (SLUB)
- `kmem_cache` with per-CPU slab (lockless fast path via `cmpxchg` on freelist + tid), per-node partial lists
- `kmalloc` size classes `kmalloc-8…`, plus `kmalloc-cg-*`, dedicated caches for hot objects
- SLAB removed (6.8), SLOB removed (6.4) → SLUB only
- Debug: `slub_debug=FZPU`, `/sys/kernel/slab/`, `slabtop`, KASAN, KFENCE (low-overhead sampling), kmemleak
- **`kvmalloc`** (kmalloc → fallback vmalloc), `vmalloc` (virtually contiguous, page-table cost, can't be used for DMA directly without mapping)

## 14. Contiguous Memory
- Buddy max order (typically order 10 → 4 MB) limits `kmalloc`
- **CMA**: reserved region still usable for movable pages; migrated out when device needs contiguous block (camera, display, codecs – common on SoCs)
- IOMMU lets scattered pages look contiguous to device (reduces CMA need)

## 15. Address Space Layout
- x86-64: 47-bit user / 48-bit kernel (5-level: 56/57-bit); canonical addresses
- ARM64: `TTBR0_EL1` user, `TTBR1_EL1` kernel; VA 39/48/52 bit; granule 4K/16K/64K
- **Direct map** (linear map of all RAM), vmalloc area, vmemmap (struct page array), modules, fixmap
- KASLR randomises base; ASLR for user; guard pages for stack
- Page-table levels: pgd → p4d → pud → pmd → pte; each page-table page itself allocated and accounted

## 16. Misc Features Worth Knowing
- **KSM** (same-page merging; VMs), **memory hotplug** (`ZONE_MOVABLE`), **DAMON** (access monitoring), **userfaultfd**, **memfd/`MFD_SECRET`**, **`process_madvise`/`MADV_COLD/PAGEOUT`** (Android), `mremap`, `madvise(DONTNEED/FREE)`
- Memory cgroups + PSI + lmkd stack (mobile)
- Kernel stacks: vmalloc'd with guard pages (`VMAP_STACK`), 16 KB x86-64, 16 KB ARM64

---

## Senior Interview Questions

**Q1. What happens on malloc(1 GB) then touching one byte?**
- `brk`/`mmap` creates VMA only; first touch → page fault → zero page/anon page alloc → PTE install; RSS grows by 4 KB (or 2 MB if THP).

**Q2. kswapd vs direct reclaim?**
- kswapd: async background when below `low`; direct reclaim: allocating task stalls when below `min`/fast path fails.

**Q3. Why can `GFP_KERNEL` deadlock in a block driver/FS?**
- Reclaim may issue IO/FS ops that need the same locks → use `GFP_NOIO`/`NOFS` or scoped APIs.

**Q4. Page cache vs buffer cache vs swap cache?**
- Page cache: file data (now unified with buffers via folios); swap cache: pages in transition to/from swap.

**Q5. Explain TLB shootdown on x86 vs ARM64.**
- x86: IPIs and software-coordinated flush; ARM64: hardware broadcast TLBI + DSB.

**Q6. THP: pros/cons?**
- Pros: fewer TLB misses; cons: compaction latency, memory bloat, fragmentation; tune per workload.

**Q7. Why do DMA pins need special handling (pin_user_pages)?**
- Device writes without CPU page table awareness; page must not move/swap/COW during DMA.

**Q8. What does OOM killer score and how can you protect a process?**
- RSS+swap+PT; `oom_score_adj=-1000`, cgroup limits, `memory.min/low`, `oom_score_adj` via systemd `OOMScoreAdjust`.

**Q9. What is CMA and when needed?**
- Large physically contiguous allocation for non-scatter-gather devices; reserved but lendable to movable pages.

**Q10. `mmap_lock` contention symptom and fix?**
- Multi-threaded `munmap`/`mmap` + page faults stall; use per-VMA locks (new kernels), reduce mmap churn, arena allocators, huge pages.

**Q11. Process RSS keeps growing but heap profiler shows no leak – why?**
- Page cache/mmap'd files, fragmentation in allocator (arenas), THP, shared memory, kernel memory (slab) outside process; check `smaps_rollup`, `/proc/meminfo`, `slabtop`.

**Q12. How does lmkd/PSI differ from kernel OOM?**
- Userspace kills early on pressure signals to keep UI responsive; kernel OOM is last resort.

---

## Quick Revision
```
Alloc path : pcp -> buddy -> kswapd wake -> compaction -> direct reclaim -> OOM
Watermarks : min < low < high (kswapd wakes at low, stops at high)
Reclaim    : LRU/MGLRU, workingset refault, writeback throttling, shrinkers
TLB        : x86 IPI shootdown, ARM64 TLBI broadcast, PCID/ASID
Pinning    : pin_user_pages + FOLL_LONGTERM, CMA/MOVABLE migrate first
VMAs       : maple tree + per-VMA locks; mmap_lock rwsem
```
