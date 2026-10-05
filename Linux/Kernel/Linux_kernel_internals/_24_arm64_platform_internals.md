# Chapter 24 – ARM64 & Platform Internals (x86 comparison)

> Extends `_13_linux_booting.md` and `8_interrupts.md`. Priority: ★★★★★ for Qualcomm / ARM / Broadcom, ★★★★☆ for AMD / Intel (know the x86 equivalents)

---

## 1. Exception Levels & Worlds
| EL | Typical software |
|---|---|
| EL0 | user applications |
| EL1 | OS kernel (Linux) |
| EL2 | hypervisor (KVM; Linux can run at EL2 with VHE) |
| EL3 | secure monitor (Trusted Firmware-A BL31) |
- **TrustZone**: Secure vs Non-secure worlds (NS bit); TEE (OP-TEE / Qualcomm QTEE) runs at S-EL1, trusted apps at S-EL0; S-EL2 with Secure Partitions (FF-A)
- Transitions: `SVC` (EL0→EL1), `HVC` (→EL2), `SMC` (→EL3)
- **x86 comparison**: rings 0-3 (kernel ring 0, user ring 3); VMX root/non-root; SMM (ring -2); Intel TXT/SGX/TDX, AMD SEV-SNP

## 2. Boot Chain (ARM64)
```
BootROM -> BL1/PBL -> BL2 (SBL/XBL on Qualcomm) -> BL31 (EL3 runtime: PSCI, SMC) -> BL32 (TEE) -> BL33 (U-Boot/UEFI/ABL)
 -> Linux Image (+ DTB / ACPI) -> start at EL2 or EL1 (head.S: MMU off, x0 = DTB pointer) -> primary_entry -> start_kernel
```
- Image header (`Image`): magic, text_offset, flags; entry with MMU off, D-cache off
- `head.S`: set up exception level (drop to EL1 or stay in EL2/VHE), create identity map, enable MMU (`SCTLR_EL1.M`), relocate (KASLR), jump to `start_kernel`
- Secondary CPUs: **PSCI `CPU_ON`** via SMC/HVC → enter `secondary_entry` → `secondary_start_kernel`
- **x86**: BIOS/UEFI → bootloader → real/long mode switch → `startup_64` → `start_kernel`; secondaries via **INIT-SIPI-SIPI** (APIC)

## 3. PSCI & Power
- **PSCI** (Power State Coordination Interface): `CPU_ON/OFF`, `CPU_SUSPEND`, `SYSTEM_SUSPEND`, `SYSTEM_RESET/OFF`, `AFFINITY_INFO`
- cpuidle with `psci_idle` states described in DT (`idle-states`); hierarchical power domains (genpd) for cluster states
- System suspend: **s2idle** (software idle, universal) vs `mem` (S3 via PSCI SYSTEM_SUSPEND); wake sources/IRQs (`enable_irq_wake`)
- Runtime PM: `pm_runtime_get_sync()/put()`, autosuspend; drivers must hold references around register access; clk/regulator/power-domain managed by PM core
- DVFS: OPP tables, `cpufreq` (schedutil), `devfreq`, interconnect (ICC) bandwidth votes, thermal framework (trip points, cooling devices)
- **x86 equivalents**: ACPI P/C/S-states, intel_pstate/amd_pstate, `intel_idle`, RAPL

## 4. MMU & Page Tables (AArch64)
- Two base registers: **TTBR0_EL1** (user, low VA) and **TTBR1_EL1** (kernel, high VA); `TCR_EL1` configures sizes/granule
- Granules: 4 KB (4 levels, 48-bit; 5th with LVA 52-bit), 16 KB, 64 KB (3 levels) → huge mappings: 2 MB/1 GB (4K), 32 MB/512 MB (16K/64K), + **contiguous-PTE** hint (16×)
- **ASID** in TTBR / TCR (8 or 16-bit) → tagged TLB; **VMID** for stage 2
- Descriptor bits: AF (access flag), AP (permissions), UXN/PXN (never-execute), SH (shareability), AttrIndx → `MAIR_EL1` memory attributes; software dirty bit (DBM hardware dirty with FEAT_HAFDBS)
- **Memory types**: Normal (WB/WT/NC), Device-nGnRnE/nGnRE/nGRE/GRE
- Security features: **PAN** (privileged access never), **PXN**, **UAO**, **BTI**, **PAC** (pointer auth), **MTE** (memory tagging), **KPTI** (only on CPUs vulnerable to Meltdown), **Spectre mitigations** (`SMCCC_ARCH_WORKAROUND`)
- TLB maintenance: `TLBI VAE1IS, ASIDE1IS, VMALLE1IS` + `DSB ISH` + `ISB`; **no IPIs** needed (broadcast)
- **x86**: CR3 (single root), PCID, 4/5-level, PAT/MTRR, SMEP/SMAP (≈ PXN/PAN), CET (≈ BTI/PAC shadow stack), IBRS/retpoline

## 5. Exceptions & Interrupts (AArch64)
- Vector table `VBAR_EL1`: 16 entries (4 sources × sync/IRQ/FIQ/SError); sources: current EL SP0/SPx, lower EL AArch64/AArch32
- `ESR_EL1` (EC: exception class, ISS: details), `FAR_EL1` (fault address), `ELR_EL1` (return address), `SPSR_EL1` (saved PSTATE)
- Kernel entry code: `kernel_ventry`, `el0_svc` → `el0_svc_common` → syscall table (`x8` = syscall nr, `x0-x5` args, `svc #0`)
- PSTATE.DAIF masks: **D**ebug, **A** (SError), **I** (IRQ), **F** (FIQ)
- **SError**: asynchronous external abort (e.g. bus error from posted write, RAS errors) – can be hard to attribute
- **Pseudo-NMI**: use GIC priority masking (`PMR`) to deliver NMI-like IRQs (perf, watchdog) when normal IRQs masked
- **x86**: IDT, `swapgs`, `syscall/sysret`, error codes, `#PF` CR2, NMI/IST stacks

## 6. GIC (Generic Interrupt Controller) – GICv3/v4
- Interrupt types: **SGI** (0-15, software-generated; IPIs), **PPI** (16-31, per-CPU peripherals e.g. arch timer, PMU), **SPI** (32+, shared peripherals), **LPI** (8192+, message-based, MSI/MSI-X via **ITS**)
- Components:
  - **Distributor (GICD)**: SPI routing/priority/enable
  - **Redistributor (GICR)**: per-CPU, PPI/SGI/LPI config; LPI tables in memory
  - **CPU interface** via system registers (`ICC_IAR1_EL1` acknowledge, `ICC_EOIR1_EL1` end-of-interrupt, `ICC_PMR_EL1` priority mask)
  - **ITS**: translates (DeviceID, EventID) → LPI + target collection (CPU); command queue
- Affinity routing (`GICD_IROUTER` with MPIDR), priority & preemption, `EOImode` split (priority drop vs deactivate – used with threaded/virtual IRQs)
- **GICv4**: direct injection of virtual LPIs to vCPUs (no hypervisor trap)
- Linux: `irq_domain` hierarchy: `GICv3 → ITS → PCI-MSI domain`; `irq_chip` ops: `irq_mask/unmask/eoi/set_affinity`; flow handlers `handle_fasteoi_irq`, `handle_percpu_devid_irq`
- **x86**: legacy PIC → IOAPIC → LAPIC/x2APIC; MSI as memory write to LAPIC address; IPIs via ICR; vector allocation `vector_irq`; interrupt remapping

## 7. Timers & Time
- **Generic Timer**: system counter `CNTVCT_EL0` (fixed frequency, typically 19.2 MHz / 1 GHz on newer), per-CPU comparators: physical (CNTP), virtual (CNTV), hypervisor timers → PPI interrupts
- `clocksource` = arch counter; `clockevent` = arch timer; **vDSO** reads `cntvct` directly (no syscall for `clock_gettime`)
- **x86**: TSC (invariant), LAPIC timer (TSC-deadline), HPET, `rdtsc` in vDSO

## 8. Caches, Coherency & Barriers (ARM)
- Levels: L1 I/D (private), L2 (per core/cluster), L3/SLC (shared), system-level interconnect (**CCI / CMN-600/700** with CHI protocol); DynamIQ clusters
- Cache ops by VA: `DC CVAC/CVAU/CIVAC/IVAC/ZVA`, `IC IVAU/IALLU`; **PoU** vs **PoC**
- **Shareability domains**: non-shareable, inner, outer; coherence maintained within inner domain
- `dmb` (ordering), `dsb` (completion), `isb` (context sync); LDAR/STLR/LDAPR; LSE atomics (see Ch. 16)
- SMP boot: caches enabled before joining coherent domain (`SMPEN` in CPUECTLR on some cores)
- Instruction/data coherency for modules, kprobes, JIT: clean D to PoU → `dsb ish` → invalidate I → `dsb ish; isb`

## 9. Firmware Interfaces
- **Device Tree (DT)**: describes non-discoverable hardware (embedded, Qualcomm mobile/auto): `compatible`, `reg`, `interrupts` (GIC cells: type, number, flags), `clocks`, `resets`, `power-domains`, `iommus`, `dma-coherent`, `status`; DT overlays; bindings in `Documentation/devicetree/bindings`; platform/of-driver matching via `of_match_table`
- **ACPI** (servers: SBSA/SBBR, Arm SystemReady; also x86): `MADT` (GIC/APIC), `GTDT` (timers), `IORT` (IOMMU/ITS/PCI mapping), `DSDT/SSDT` (AML devices, `_HID`, `_CRS`), `FADT`, `SRAT/SLIT` (NUMA), `MCFG` (PCIe ECAM), `_DSD` properties; `_CCA` (DMA coherency)
- **SCMI/SCPI** (firmware performance/power/clock control via mailbox + shared memory), **PSCI**, **SMCCC**
- Qualcomm specifics: RPMh (resource power manager) votes, `qcom-scm` SMC calls, remoteproc for DSPs (Hexagon: ADSP/CDSP/MPSS), SMEM/QMP/GLINK/APR IPC to subsystems, `interconnect` bandwidth votes, `pd-mapper`, Gunyah/QHEE hypervisor, minidump
- Broadcom specifics: BCM2xxx (Raspberry Pi: VideoCore mailbox), Broadcom switch/NIC SDKs, VideoCore firmware boots first
- ARM server (Neoverse): SystemReady, SBSA, UEFI + ACPI

## 10. Virtualization on ARM (KVM)
- Stage-1 (guest OS) + **Stage-2** translation (IPA→PA) with VMID; HCR_EL2 traps; **VHE** (host kernel at EL2)
- vGIC (GICv3/v4), virtual timer (CNTV), `kvm_vcpu_run`, `__kvm_vcpu_run` world-switch, `KVM_RUN` ioctl → exit reasons (MMIO, HVC, WFx, sysreg trap)
- vs x86: VMX/SVM, EPT/NPT, VMCS/VMCB, APICv/AVIC
- Memory: stage-2 faults → `kvm_handle_guest_abort` → `gfn_to_pfn` (uses `mmu_notifier`)
- Virtio, vhost, vfio-pci passthrough with SMMU stage-2/nested

## 11. Linux Driver Framework Pieces on SoCs
- **Common clock framework** (`clk_get`, `clk_prepare_enable`, `clk_set_rate`), **reset controller**, **regulator**, **pinctrl** (mux/config states), **GPIO (gpiod)**, **genpd** (power domains), **interconnect**, **regmap** (MMIO/I2C/SPI register abstraction + caching), **syscon**, **mailbox**, **remoteproc/rpmsg**, **thermal**, **cpufreq/devfreq**, **extcon**, **IIO**, **V4L2 / DRM-KMS / ASoC**
- Probe pattern: get resources (`devm_*`) → enable clocks/regulators/power → reset → map regs → request IRQ → register with subsystem → `pm_runtime_enable`
- Defer: `dev_err_probe()` handles `-EPROBE_DEFER` logging; `fw_devlink` orders probing automatically from DT links
- Common bug: accessing registers before enabling clock/power domain → synchronous abort/bus hang

## 12. x86-Specific Items Worth Knowing (AMD/Intel interviews)
- Rings, GDT/IDT/TSS, `swapgs`, `syscall`/`sysret`, `sysenter`; per-CPU via `%gs`
- Paging: CR0/CR3/CR4, PCID, PAT, large pages (2M/1G), NX; SMEP/SMAP, KPTI (`isolation`), IBRS/IBPB/STIBP/retpoline (Spectre), MDS mitigations
- APIC/x2APIC/IOAPIC, MSI/MSI-X, **IRQ remapping**, NMI (+IST), MCE (`#MC`) & RAS, SMM/SMIs (latency source), ACPI (SRAT/SLIT/DMAR/IVRS)
- Intel VT-x/VT-d/TDX; AMD-V (SVM)/AMD-Vi IOMMU/SEV-SNP; AMD NPS/CCX/CCD topology & Infinity Fabric (NUMA effects)
- Perf counters: Intel PMU/PEBS/LBR, AMD IBS; uncore events

---

## Senior Interview Questions

**Q1. Describe ARM64 exception levels and what runs at each.**
- EL0 user, EL1 kernel, EL2 hypervisor, EL3 secure monitor (TF-A); Secure/Non-secure worlds.

**Q2. Walk through a syscall on ARM64.**
- `svc #0` from EL0 → vector `el0_sync` → ESR EC=SVC64 → `el0_svc` → syscall table by `x8` → return via `eret` with `ELR/SPSR`.

**Q3. Explain GICv3 interrupt delivery for a PCIe MSI-X.**
- Device writes to ITS `GITS_TRANSLATER` (DeviceID/EventID) → ITS maps to LPI + collection → redistributor of target CPU → CPU interface → `ICC_IAR1_EL1` ack → handler → `EOIR`.

**Q4. How are secondary CPUs brought up on ARM64?**
- Primary calls PSCI `CPU_ON` (SMC/HVC) with entry point; firmware powers CPU, which enters `secondary_entry`, enables MMU, joins `secondary_start_kernel`.

**Q5. TLB invalidation differences between ARM64 and x86?**
- ARM64: broadcast TLBI + DSB (hardware); x86: INVLPG/INVPCID + IPI shootdown coordinated in software.

**Q6. DT vs ACPI?**
- DT static hardware description for embedded; ACPI includes AML methods for dynamic power/config, standard for servers; both feed same driver model.

**Q7. What is a synchronous external abort and how would you debug it?**
- CPU access got error response from bus (unclocked/unpowered/unmapped/secure-only); check clocks/regulators/power domains, DT address, firmware ownership; ESR/FAR decode.

**Q8. What are SGI, PPI, SPI, LPI?**
- Software-generated (IPI), private peripheral (timer/PMU), shared peripheral (wired), locality-specific peripheral (message-based).

**Q9. How does KPTI/Meltdown mitigation work on ARM64 and what's the cost?**
- Unmaps kernel from user TTBR0 view using trampoline; extra TTBR switch + TLB (ASID) cost; enabled only on affected cores.

**Q10. How does runtime PM relate to driver correctness?**
- Hardware may be gated off when idle; driver must `pm_runtime_get` before register access/IRQ handling and `put` after; mismatch → aborts or leaks power.

**Q11. How does KVM on ARM64 handle guest memory?**
- Stage-2 page tables (VMID) populated on faults using host pages; mmu_notifier keeps stage-2 consistent; vGIC/virtual timer accelerate.

**Q12. Compare x86 and ARM64 memory models in one sentence.**
- x86 TSO (only store→load reorder); ARM64 weakly ordered with acquire/release instructions and explicit `dmb`.

---

## Quick Revision
```
EL0 user | EL1 kernel | EL2 hyp | EL3 monitor (TF-A, PSCI)
MMU: TTBR0/1, ASID, granule 4/16/64K, MAIR attrs, TLBI broadcast (no IPI)
Exceptions: VBAR_EL1, ESR/FAR/ELR/SPSR, svc x8
GICv3: SGI/PPI/SPI/LPI, GICD/GICR/ITS, ICC_* sysregs
Firmware: DT / ACPI (IORT, MADT, GTDT), PSCI, SCMI, SMCCC
SoC drivers: clk, reset, regulator, pinctrl, genpd, runtime PM, regmap, interconnect
```
