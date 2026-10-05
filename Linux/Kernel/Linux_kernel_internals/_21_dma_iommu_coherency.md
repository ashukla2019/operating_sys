# Chapter 21 – DMA API, IOMMU/SMMU, PCIe & Cache Coherency

> Extends `_12_device_driver.md` §42-47 and `9_block_io.md`. Priority: ★★★★★ for semiconductor roles (AMD / Qualcomm / Intel / Broadcom / ARM)

---

## 1. Address Types
| Name | Meaning |
|---|---|
| Virtual (VA) | CPU address (user / kernel) |
| Physical (PA) | address on CPU memory bus |
| Bus / DMA address (IOVA) | address **device** uses; = PA when no IOMMU, else IOVA translated by IOMMU |
- Never pass `virt_to_phys()` to a device – use the DMA API (`dma_addr_t`)

## 2. DMA API (core calls)
```c
dma_set_mask_and_coherent(dev, DMA_BIT_MASK(64));   // what the device can address (fallback 32)
buf = dma_alloc_coherent(dev, size, &dma_handle, GFP_KERNEL);  // long-lived, descriptors/rings
dma_free_coherent(dev, size, buf, dma_handle);

dma_addr_t d = dma_map_single(dev, ptr, len, DMA_TO_DEVICE); // streaming, per-I/O buffer
if (dma_mapping_error(dev, d)) ...
dma_unmap_single(dev, d, len, DMA_TO_DEVICE);

n = dma_map_sg(dev, sgl, nents, dir);   // scatter-gather; n may be < nents (IOMMU merging)
dma_sync_single_for_cpu / _for_device   // reuse mapped buffer: hand ownership back & forth
dma_pool_create/alloc                   // small coherent objects
dma_map_page, dma_map_resource (MMIO of other device, P2P)
```
- **Coherent (consistent)** mappings: both CPU and device see updates without explicit sync (uncached/WC or HW-coherent) – use for descriptor rings
- **Streaming** mappings: cached CPU memory; **ownership model** – between map and unmap device owns buffer (CPU must not touch unless `sync_for_cpu`)
- Direction matters: `TO_DEVICE` (clean cache), `FROM_DEVICE` (invalidate), `BIDIRECTIONAL` (both; avoid when possible)
- `dma_map_single` only for **linear kernel addresses** (kmalloc/page) – **not** stack, `vmalloc`, or module-static memory
- `dma_alloc_attrs` flags: `DMA_ATTR_NO_KERNEL_MAPPING`, `DMA_ATTR_WRITE_COMBINE`, `DMA_ATTR_FORCE_CONTIGUOUS`, `DMA_ATTR_SKIP_CPU_SYNC`
- `devm_`-style and `dmam_alloc_coherent` for managed lifetime

## 3. Cache Coherency in DMA
- **Hardware-coherent system** (typical x86, ARM with ACE-lite/CCI/CMN and `dma-coherent` DT / ACPI `_CCA=1`): DMA snoops CPU caches → map/unmap mostly no-ops
- **Non-coherent system** (many ARM SoCs, embedded): kernel must **clean** (write back) before device reads, **invalidate** before CPU reads device-written data
  - `arch_sync_dma_for_device()` / `arch_sync_dma_for_cpu()` → `DC CVAC/CIVAC/IVAC` loops on ARM64
- Gotchas:
  - Buffer **cache-line alignment**: sharing a cache line with unrelated data → invalidate destroys neighbouring dirty data (`ARCH_DMA_MINALIGN`, ARM64 `kmalloc` min-align 128)
  - Don't touch buffer between map and `sync_for_cpu`
  - Speculative CPU prefetch can refill lines after invalidate → hence invalidate **again** at `unmap/sync_for_cpu`
- Detect/validate: `CONFIG_DMA_API_DEBUG`, `dma-debug` reports unmatched map/unmap, wrong direction, leaks

## 4. Ordering With Descriptors (driver idiom)
```c
// producer (CPU) -> consumer (device)
fill_descriptor(desc);          // addr, len, flags except OWN
dma_wmb();                      // descriptor body visible before OWN bit
desc->ctrl |= OWN;
writel(tail, hw->doorbell);     // implies wmb before MMIO store

// completion (device) -> CPU
if (!(READ_ONCE(desc->ctrl) & OWN)) {
    dma_rmb();                  // read ctrl before reading rest of descriptor
    use(desc->len);
}
```
- See Ch. 16 §14 for `readl/writel` guarantees

## 5. bounce buffers – SWIOTLB
- Device can't address the buffer (mask too small, or **IOMMU-less + >4 GB RAM**, or confidential-compute guests) → copy via low-memory **bounce buffer**
- Cost: extra copy, limited pool (`swiotlb=` size), fragmentation, latency
- Also used for security (untrusted devices, `swiotlb=force`, TDX/SEV guests)

## 6. IOMMU Fundamentals
- Translates **IOVA → PA** for DMA-capable devices + permission bits (R/W)
- Benefits:
  1. **Isolation / protection** (malicious or buggy device can't scribble kernel memory)
  2. **Scatter-gather as contiguous** (no CMA / bounce buffers)
  3. **32-bit devices** reach any RAM
  4. **Device assignment** to VMs/userspace (VFIO)
  5. Shared virtual addressing (SVA/SVM)
- Hardware: Intel **VT-d** (DMAR), AMD **AMD-Vi**, ARM **SMMUv2/v3**, Apple DART, Qualcomm SMMU variants (`arm-smmu-qcom`)
- Costs: IOTLB misses, page-table walks on DMA path, invalidation cost, memory for tables

## 7. Linux IOMMU Framework
- `struct iommu_domain` = an address space (page tables); `iommu_group` = devices that can't be isolated from each other (ACS/aliasing)
- **DMA domains**: `iommu-dma` allocates IOVA (`iova_domain` with rbtree + per-CPU caches), maps/unmaps on `dma_map_*`
  - Modes: **strict** (flush IOTLB on unmap – safe), **lazy/deferred** (batch flush queue – faster, short stale window), `iommu.passthrough=1` (identity)
- `iommu_map()/unmap()`, `iommu_attach_device()`, `iommu_iova_to_phys()`
- Kernel cmdline: `intel_iommu=on`, `amd_iommu=on`, `iommu=pt` (passthrough for host devices, translation for VFIO), `iommu.strict=0/1`
- Since 6.x: **iommufd** (new userspace API replacing VFIO type1 gradually), nested translation

## 8. ARM SMMUv3 (very likely at Qualcomm/ARM/Broadcom)
- **StreamID** identifies the master (device / PCIe RID mapped via `iommu-map` / `iommu-map-mask` in DT or ACPI IORT)
- Data structures:
  - **Stream Table** (STE: per StreamID – points to context)
  - **Context Descriptor table** (CD: per SubstreamID/PASID – stage-1 page table base `TTBR`, ASID)
  - Stage 1 (VA→IPA, owned by guest/OS) + Stage 2 (IPA→PA, hypervisor)
- Queues: **Command queue** (CMD_SYNC, TLBI, CFGI), **Event queue** (faults), optional **PRI queue** (page requests)
- Features: ATS/PRI, PASID, HTTU (hardware dirty/access), MSI-based completion
- **Faults**: translation fault, permission fault → event queue → `arm_smmu_evtq_thread` logs (`Unhandled context fault`)
- Qualcomm: SMMU often **shared with firmware/hypervisor**; context banks (SMMUv2) limited; "split" ownership

## 9. SVA / PASID / ATS / PRI
- **PASID** (Process Address Space ID) tags DMA with process context → device uses **same CPU page tables** (SVA)
- **ATS**: device caches translations (device-side TLB); needs invalidation via `mmu_notifier` → IOMMU → device
- **PRI**: device requests page-fault handling (on-demand paging for DMA)
- Use: GPUs, NPUs, RDMA, accelerators (DSA/IAX), `iommu_sva_bind_device()`

## 10. VFIO & SR-IOV & Virtualisation
- **VFIO**: expose device to userspace/VM safely — group → container (IOMMU domain) → device regions/IRQs via `ioctl`; DPDK/SPDK, QEMU passthrough
- **SR-IOV**: PF + many VFs (each with own BARs, queues, RID); VFs assigned to VMs; PF driver manages (`sriov_numvfs`)
- **Interrupt remapping** (Intel IR, AMD IRTE, ARM GICv3 ITS/GICv4 vLPI direct injection) – safe MSI delivery to guests
- **Mediated devices / vDPA / virtio** for sharing without full passthrough

## 11. PCIe Essentials (driver view)
- Topology: Root Complex → Switches → Endpoints; each function has **BDF**; config space 256 B (legacy) / 4 KB (PCIe, via **ECAM**)
- **BARs**: MMIO/IO windows; `pci_iomap()`, `pcim_iomap_regions()`; BAR sizing by write-1s
- Enable flow: `pci_enable_device_mem()` → `pci_request_regions()` → `pci_set_master()` → `dma_set_mask…` → map BARs → `pci_alloc_irq_vectors(MSI/MSI-X)` → `request_irq`/`devm_request_threaded_irq`
- **TLPs**: Memory Read/Write (posted), Config, Completion; **posted writes** have no completion → read-back to flush; ordering rules (posted passes non-posted, Relaxed Ordering/ID-based ordering bits)
- **MSI/MSI-X**: interrupt = memory write to address (APIC / GIC ITS); per-vector affinity; no sharing, no level semantics
- **Link**: width/speed (gen3/4/5/6), **ASPM** (L0s/L1), **AER** (correctable / non-fatal / fatal errors, `pci_error_handlers`: `error_detected`, `slot_reset`, `resume`), **FLR**, hotplug (native/ACPI), `pci_reset_function`
- **MPS / MRRS** (max payload / read request size) tuning affects throughput
- **P2PDMA**: device-to-device DMA (`pci_p2pdma_*`), needs switch/ACS support
- **CXL**: cache-coherent PCIe-based (CXL.io / .cache / .mem) – type 1/2/3 devices
- Debug: `lspci -vvv`, `setpci`, `/sys/bus/pci/devices/*/`, `dmesg | grep -i aer`, `lspci -t`

## 12. Memory Types for MMIO (ARM64 / x86)
- ARM64 **Device-nGnRnE/nGnRE/GRE**, Normal-NC (write-combining style), Normal cacheable
- `ioremap()` = Device-nGnRE (strict, no gathering/reordering/early write-ack variants); `ioremap_wc()` = Normal-NC (allows merging – framebuffers, doorbell batches); `memremap()` for RAM-like regions
- x86: UC / WC / WT / WB via PAT/MTRR
- Never `memcpy` to `__iomem` – use `memcpy_toio()/memcpy_fromio()`, `iowrite32`, `readl`

## 13. Descriptor Rings & Doorbells (NIC/NVMe/GPU style)
- Producer/consumer indices (head/tail) in coherent memory or device registers
- Doorbell coalescing, batched writes, interrupt moderation, NAPI/polling
- NVMe: SQ/CQ pairs per CPU, phase tag in CQ entry (no need for separate valid flag), doorbell stride, MSI-X per CQ, `io_uring` passthrough
- Common bugs: missing barrier before doorbell, wrong endianness, stale cache (non-coherent), double free of DMA mapping, mapping stack buffers, using `dma_addr_t` as pointer

## 14. dma-buf & Fences (GPU / multimedia – AMD, Intel, Qualcomm)
- **dma-buf**: kernel object to share buffers across drivers/processes (fd): exporter (`dma_buf_export`), importer (`dma_buf_attach`, `dma_buf_map_attachment` → sg_table), `mmap`, CPU access hooks (`begin/end_cpu_access` for coherency)
- **dma-fence / sync_file**: completion signalling across drivers; implicit vs explicit sync (`DMA_RESV` reservation objects)
- DRM/GEM/TTM: GPU memory managers; `drm_gpuvm`; `mmu_notifier`/HMM for SVM
- Android: ION → DMA-BUF heaps

## 15. Debug Recipes
| Problem | Approach |
|---|---|
| Data corruption on ARM, fine on x86 | non-coherent DMA/cache sync or missing `dma_wmb/rmb`; check `dma-coherent` DT, `CONFIG_DMA_API_DEBUG` |
| `DMAR: DRHD: handling fault status`/`SMMU unhandled context fault` | device wrote to unmapped IOVA → UAF/early unmap/bad length; log shows IOVA & StreamID; check unmap ordering, DMA after reset |
| Throughput lower with IOMMU | strict invalidation, small mappings → use `iommu.passthrough`/lazy, larger pages, `dma_map_sg` coalescing, hugepage IOVA |
| Device hangs after FLR/reset | quiesce DMA first, drain rings, `pci_reset_function`, check AER |
| `swiotlb buffer is full` | mask too low, too many mappings; raise `swiotlb=`, fix mask |
| Bus error / Synchronous External Abort (ARM) | MMIO access to powered-off block (clock/power domain/regulator not enabled) |

---

## Senior Interview Questions

**Q1. Coherent vs streaming DMA mapping?**
- Coherent: shared long-lived, no explicit sync; streaming: per-transfer, cached, needs map/sync/unmap with ownership.

**Q2. Why is `virt_to_phys` wrong for DMA?**
- With IOMMU/bounce/bus offsets, device address ≠ PA; DMA API returns correct bus address and handles cache maintenance.

**Q3. How does the kernel make DMA safe on a non-coherent ARM SoC?**
- Cache clean/invalidate at map/sync/unmap per direction; aligned buffers; coherent allocations are uncached/WC.

**Q4. What does the IOMMU buy you? What does it cost?**
- Isolation, SG remap, 32-bit devices, virtualisation, SVA; costs: IOTLB misses, invalidation overhead, setup cost.

**Q5. Strict vs lazy IOTLB invalidation?**
- Strict: invalidate on every unmap (safe, slower). Lazy: batched flush (faster, brief window where device may still DMA to freed IOVA).

**Q6. Explain SMMUv3 translation for a PCIe device.**
- RID → StreamID (IORT/DT) → STE → CD (stage 1) → page table walk (+ stage 2) → PA; faults via event queue; invalidations via command queue.

**Q7. What is an IOMMU group?**
- Smallest set of devices that can't be isolated from each other (shared requester IDs / no ACS) – passthrough granularity.

**Q8. How does a driver safely tear down DMA?**
- Quiesce device (disable bus master / reset), wait for in-flight DMA, then unmap/free; never free before device stops.

**Q9. What is the posted-write problem?**
- MMIO write may sit in bridge buffers; subsequent action assumes completion; read back a register to flush.

**Q10. MSI vs MSI-X vs legacy INTx?**
- INTx: shared, level, sideband; MSI: up to 32 vectors, contiguous, single address; MSI-X: up to 2048, per-vector address/data & masking.

**Q11. How do PASID/SVA change the programming model?**
- Device uses process VA directly; no pinning/mapping; requires ATS/PRI and mmu_notifier invalidation.

**Q12. A DMA'd packet buffer shows stale data on ARM only. Root cause candidates?**
- Missing `dma_sync_*for_cpu`, `DMA_FROM_DEVICE` invalidate absent, buffer not cache-line aligned, `dma-coherent` property wrong, missing `dma_rmb()` before reading payload after status.

---

## Whiteboard Drill
- Draw CPU → cache → interconnect → DRAM, device → (IOMMU) → interconnect; mark where snoop happens and where cache ops are needed
- Draw SMMUv3 tables: StreamID → STE → CD → TTBR → PA
- Write NIC RX refill loop: map pages, fill descriptor, barrier, doorbell, completion handling, unmap/sync

## Quick Revision
```
Coherent = rings/descriptors   Streaming = data buffers (map -> device owns -> sync/unmap)
Non-coherent SoC: clean before device read, invalidate before CPU read
IOMMU: IOVA->PA, isolation, SG, VFIO, SVA; ARM SMMUv3 = StreamID->STE->CD->PT
Ordering: dma_wmb before OWN/doorbell, dma_rmb after status; writel implies wmb
PCIe: BAR, MSI-X, posted writes (read-back), AER, FLR, MPS/MRRS
```
