# Chapter 18 – NUMA Deep Dive

> Extends `3_memory_mgmt.md` §49-50 and `_10_scheduling.md` §27. Priority: ★★★★★ (explicitly in your target list)

---

## 1. What & Why
- **NUMA** = memory access latency/bandwidth depends on *which CPU* touches *which memory*
- Cause: memory controllers are attached to sockets / chiplets; remote access crosses an interconnect
  - Intel: UPI; AMD: Infinity Fabric; ARM servers: CMN mesh + CCIX/CXL/proprietary links
- Remote access: higher latency (commonly ~1.3-2x+ local), lower bandwidth, interconnect contention
- Matters for: databases, in-memory stores, packet processing, storage targets, AI/accelerator hosts, VMs

## 2. Topology Sources
- **ACPI**: `SRAT` (CPU/memory → node), `SLIT` (distance matrix), `HMAT` (latency/bandwidth), `CEDT/CFMWS` (CXL)
- **Device Tree (ARM/embedded)**: `numa-node-id`, `numa-distance-map-v1`
- Distances: 10 = local, 20/21/... = remote; used for fallback ordering
- **Sub-NUMA modes**: Intel SNC, AMD NPS1/NPS2/NPS4 (split socket into multiple nodes) – lower latency, more nodes
- **Memory-less nodes** (CPU only) and **CPU-less nodes** (CXL / HBM / PMEM) exist
- Tools: `numactl -H`, `lscpu`, `lstopo` (hwloc), `/sys/devices/system/node/node*/{cpulist,meminfo,distance}`

## 3. Kernel Data Structures
- `pg_data_t` per node → `node_zones[]` (DMA, DMA32, Normal, Movable) + `zonelists`
- Per-node: **LRU lists**, `kswapd` thread, free area, slab partial lists, vmstat counters, `lruvec`
- **Zonelist** = preferred allocation order for a node (local zones first, then by distance)
- `numa_node_id()`, `cpu_to_node()`, `dev_to_node()`, `page_to_nid()`

## 4. Default Allocation Behaviour
- Policy `MPOL_DEFAULT` = **local allocation**: page comes from the node of the CPU that **first touches** it (at page-fault)
- **First-touch problem**: main thread initialises a big array → all pages land on node 0 → worker threads on node 1 run remote
  - Fix: parallel initialisation by the threads that will use the data, or explicit policy, or numa balancing
- Allocation falls back to other nodes when local is below watermark → "spill" (`numa_miss`/`numa_foreign`)
- Kernel allocations: `kmalloc_node()`, `alloc_pages_node()`, `kmem_cache_alloc_node()`, `vmalloc_node()`

## 5. Memory Policies (per task / per VMA)
| Mode | Behaviour |
|---|---|
| `MPOL_DEFAULT/LOCAL` | allocate on node of faulting CPU |
| `MPOL_BIND` | only these nodes (OOM if exhausted) |
| `MPOL_PREFERRED` | try this node, fall back |
| `MPOL_PREFERRED_MANY` | prefer set of nodes |
| `MPOL_INTERLEAVE` | round-robin pages across nodes → bandwidth spread, avoids hot node |
| `MPOL_WEIGHTED_INTERLEAVE` (6.9+) | interleave in ratio (useful with CXL tiers) |
- Syscalls: `set_mempolicy()`, `get_mempolicy()`, `mbind()` (per range), `migrate_pages()`, `move_pages()`
- CLI: `numactl --cpunodebind=0 --membind=0 ./app`, `numactl --interleave=all ./db`
- libnuma: `numa_alloc_onnode`, `numa_run_on_node`, `numa_set_preferred`
- cgroup v2 `cpuset.mems` / `cpuset.cpus` restrict nodes for a group (containers, K8s topology manager)

## 6. Automatic NUMA Balancing
- Goal: move **tasks to their memory** or **memory to their tasks** without admin tuning
- Mechanism:
  1. Scanner (`task_numa_work` via task_work on tick) periodically **unmaps pages** (marks PTE `PROT_NONE`-style "NUMA hinting") in a window of the address space
  2. Next access triggers a **NUMA hinting fault** (`do_numa_page`) → kernel records *which node* touched the page (per task & per group stats)
  3. `task_numa_fault()` accumulates → scheduler computes **preferred node** (`numa_preferred_nid`)
  4. Scheduler: migrate task toward preferred node (`task_numa_migrate`, swap with another task if needed)
  5. Memory: `migrate_misplaced_page()` moves hot page to the accessing node (rate-limited, 2-fault filter to avoid ping-pong)
- Tunables: `kernel.numa_balancing` (0/1/2), `numa_balancing_scan_delay_ms`, `scan_period_min/max_ms`, `scan_size_mb`
- Costs: extra minor faults, TLB flushes, page migration copy; can hurt latency-sensitive or well-pinned workloads → often **disabled when you pin manually**
- Observe: `/proc/vmstat` → `numa_pte_updates`, `numa_hint_faults`, `numa_hint_faults_local`, `numa_pages_migrated`; `/proc/<pid>/sched` numa stats

## 7. Scheduler & NUMA
- **Scheduling domains** have `SD_NUMA` levels; load balancing across nodes is **less eager** (cost of cache/memory loss)
- `wake_affine`, `select_idle_sibling` prefer same LLC/node
- NUMA group scheduling for tasks sharing memory (`numa_group` by faults on shared pages)
- Pin with `taskset`, `sched_setaffinity`, `cpuset`; use `isolcpus` + manual placement for RT/DPDK-type loads
- Interaction: balancing may fight memory policy – prefer one strategy

## 8. Per-Node Memory Management
- **kswapd per node**: reclaims when node free pages < watermark (not global!) → one node can be reclaiming while others idle
- `vm.zone_reclaim_mode` – reclaim locally before spilling remote (helps locality, may hurt page cache); `node_reclaim`
- **Hugepages per node**: `/sys/devices/system/node/nodeN/hugepages/...`; THP allocation prefers local
- Page cache placement: follows reading CPU's node (file shared by many nodes → one node)
- **Slab/SLUB** per-node partial lists; remote frees cost more (`kmem_cache_free` from other node)
- `memory.numa_stat` in memcg v2

## 9. Memory Tiering (CXL, HBM, PMEM)
- Slow-tier memory shows as CPU-less NUMA node
- **Demotion**: reclaim moves cold pages to slow node instead of swap (`demotion_enabled`)
- **Promotion**: NUMA balancing (mode 2 "memory tiering") promotes hot pages back
- `memory-tiers` abstraction, `HMAT` drives tier ranking; weighted interleave for bandwidth expansion

## 10. Device/IRQ Locality (driver-relevant!)
- PCIe device is attached to a node: `/sys/bus/pci/devices/.../numa_node`, `dev_to_node(dev)`
- Allocate DMA buffers, rings, per-queue structs on the device's node; `dma_alloc_coherent` uses device node
- Pin IRQs & NAPI threads / worker threads to cores **on the device's node** (`/proc/irq/N/smp_affinity`, `irqbalance` is NUMA-aware only partly)
- Pin app threads handling that NIC/NVMe queue to the same node → avoids remote DMA-completion cache misses
- RSS/XPS mapping per queue to local CPUs
- GPU/accelerator: host buffers pinned on the GPU-local node; P2P across sockets penalised

## 11. NUMA-Aware Software Design
- **Partition/shard** data per node; thread-per-core + local allocation (shared-nothing)
- Per-node caches/pools/counters; avoid global locks and globally shared hot cachelines
- **NUMA-aware locks**: hierarchical (cohort/HCLH), prefer handing lock to same-node waiter; also qspinlock's local spinning helps
- Avoid false sharing & remote atomics (cross-socket atomic ~ several hundred ns)
- Replicate read-mostly data per node (e.g. kernel text/rodata replication proposals; user-level replication)
- Interleave when access is random and bandwidth-bound; bind when locality-friendly
- Allocator: jemalloc/tcmalloc per-CPU/arena, `numa_alloc_onnode`, `MPOL_BIND` for arenas

## 12. Virtualisation
- vNUMA: guest sees topology; vCPU pinning + memory binding (`numatune`, `-numa` in QEMU)
- Mismatch (guest thinks local, host remote) = hidden slowdown; use host NUMA balancing or explicit pinning

## 13. Diagnosing NUMA Problems
| Symptom | Check |
|---|---|
| Throughput drops when threads spread across sockets | `numastat -p <pid>`, `/proc/<pid>/numa_maps` (pages per node `N0=.. N1=..`) |
| High `numa_miss`/`numa_foreign` | `numastat` – allocator spilling; local node full |
| Latency spikes & many minor faults | NUMA balancing scanning / migration; `numa_hint_faults` |
| One node OOM/reclaim while others free | `MPOL_BIND`, cpuset mems, per-node `kswapd` busy, `zone_reclaim_mode` |
| Remote cacheline contention | `perf c2c`, `perf mem`, `perf stat -e node-loads,node-load-misses` |
| Memory bandwidth imbalance | `pcm-memory` / `likwid`, `perf` uncore events |
- Method: **map topology → map threads/IRQs → map memory → measure → pin/policy → re-measure**

## 14. Common Pitfalls
- Assuming `malloc` returns memory on local node at `malloc` time (it's at **first touch**)
- Pinning threads but leaving memory on wrong node (use `--membind`/first-touch init in worker)
- `fork`/`exec` inherits policy; THP khugepaged may collapse across nodes
- Container CPU limit via CFS quota ≠ NUMA placement (needs cpuset / topology manager)
- Page cache or hugepages preallocated on single node

---

## Senior Interview Questions

**Q1. What is NUMA and how does Linux represent it?**
- Nodes with CPUs + local memory; `pg_data_t` per node, zones, zonelists; topology from ACPI SRAT/SLIT or DT.

**Q2. Where does a page get allocated by default?**
- On the node of the CPU that first touches it (local policy); falls back by zonelist distance.

**Q3. Explain first-touch and how it can hurt.**
- Single-threaded init puts all pages on one node; later parallel workers run remote. Parallel init or interleave/bind.

**Q4. How does automatic NUMA balancing work?**
- Periodic PTE unmapping → hinting faults → per-task node statistics → preferred node → task migration + page migration (rate limited).

**Q5. When would you disable NUMA balancing?**
- Manually pinned workloads, latency-critical RT, when hinting-fault overhead > benefit.

**Q6. Interleave vs bind?**
- Interleave: spreads bandwidth, avoids hotspot, no locality. Bind: strict locality, risks OOM on one node.

**Q7. How do you place driver resources NUMA-correctly?**
- `dev_to_node()`, `*_node()` allocators, IRQ affinity to local node cores, queue-per-CPU on local node.

**Q8. How does the scheduler treat NUMA?**
- SD_NUMA domains, rare cross-node balancing, preferred-node logic from fault stats, wake-affine locality.

**Q9. What is the cost of cross-socket lock contention and how to reduce it?**
- Coherence traffic through interconnect; use per-node sharding, hierarchical locks, MCS-style local spinning, reduce sharing.

**Q10. How would you debug "app is 2x slower on 2-socket machine than on 1-socket"?**
- `numactl -H`, `numastat -p`, `numa_maps`, `perf c2c/mem`, test with `--cpunodebind=0 --membind=0`, then interleave/shard; check IRQ locality and THP.

**Q11. What is a CPU-less NUMA node?**
- Memory-only node (CXL/HBM/PMEM) acting as slower tier; used for demotion/promotion.

**Q12. How does `kswapd` relate to NUMA?**
- One per node; watermarks per node/zone; imbalance causes local reclaim despite free memory elsewhere.

---

## Whiteboard Drill
- Draw 2-socket system: cores, LLC, memory controllers, interconnect, PCIe NIC on node 1; mark good vs bad placement of IRQ, ring buffers, worker threads
- Draw hinting-fault flow for a page from scan → fault → stats → migrate

## Quick Revision
```
Default   : first-touch local alloc, zonelist fallback by distance
Policies  : default/bind/preferred/interleave (+weighted), mbind/set_mempolicy
Balancing : PTE hinting faults -> preferred node -> migrate task / pages
Per node  : zones, LRU, kswapd, slab partials, hugepages
Drivers   : dev_to_node, *_node allocs, IRQ+threads on device node
Debug     : numactl -H, numastat, numa_maps, perf c2c/mem
```
