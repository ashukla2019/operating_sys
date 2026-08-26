# Linux Device Driver — Consolidated Reference

Every driver type follows the same two-phase shape. Keep this split in your head above all else:

| Phase | Question it answers | Key players |
|---|---|---|
| **A. Discovery / matching** | "Which driver should handle this hardware?" | bus subsystem, Device Tree/ACPI, `probe()`/`remove()` |
| **B. Runtime interface** | "How does an application talk to the driver?" | major/minor, `cdev`, `file_operations`, `/dev/*` |

**Don't mix these two.** Matching identity (Vendor ID, `compatible` string) and device-number identity (major:minor) are unrelated systems that happen to both point at "your driver."

The whole thing end to end, before we zoom into each piece:

```
                          Power on
                             |
                             ↓
   1.                    HARDWARE
                             |
                             ↓
   2.        PCI / USB / I2C / SPI / Platform
             (bus/subsystem detects or is told
              a device exists — Section 1)
                             |
                             ↓
   3.                   DISCOVERY
                             |
                             ↓
   4.              Device structure
             (generic struct device created —
              Section 1)
                             |
                             ↓
   5.              DRIVER MATCHING
             (Vendor ID / compatible string
              matched to a registered driver —
              Section 2)
                             |
                             ↓
   6.                    probe()
             (driver's init callback runs —
              Section 2)
                             |
                             ↓
   7.        Driver initializes hardware
             (MMIO, IRQ, DMA, resources —
              Section 2)
                             |
                             ↓
   8.  Driver registers with a kernel interface — this is
       the fork point (Section 3):
                             |
             +--------------+--------------+
             |              |              |
             ↓              ↓              ↓
          Character        Block        Network/Input
             |              |              |
             ↓              ↓              ↓
            cdev       Block subsystem   net_device /
             |                         input subsystem
             ↓              ↓              ↓
          /dev/...       /dev/...       ethX / eventX
             |              |              |
             +--------------+--------------+
                             |
                             ↓
   9.                  USERSPACE
             (application interacts via
              open/read/write, block I/O,
              or sockets — Sections 4–6)

          Character → cdev              (Section 4)
          Block     → block layer       (Section 5)
          Network   → net_device/network stack  (Section 6)
          Input     → input subsystem   (not covered in detail here)
```

Everything down to "Driver initializes hardware" (Sections 1–2) is identical no matter what kind of driver this turns out to be. The fork at the bottom is Section 3 — and Sections 4, 5, 6 each zoom into one branch of that fork.

---

## 1. Phase A — Discovery per bus type

Linux doesn't discover arbitrary hardware by itself; a bus/subsystem discovers or describes the device first. The mechanism differs by bus:

```
Bus/connection      Discovery style                    Kernel struct
─────────────────────────────────────────────────────────────────────
PCI/PCIe          Hardware self-describes;         struct pci_dev
                   kernel scans the bus for
                   Vendor ID + Device ID

USB                Hardware self-describes on       struct usb_device
                   hotplug (enumeration);
                   Vendor ID + Product ID

I2C / SPI          NOT self-describing — Linux is   struct i2c_client /
                   told a device exists at a        struct spi_device
                   given address/chip-select via
                   Device Tree / ACPI

SoC / platform     Peripherals are inside the SoC;   struct platform_device
                   no bus scan possible — Linux is
                   told via Device Tree / ACPI
```

Mental model — think of it as a transport system: the **subsystem** manages the road (how to talk to hardware over that bus), the **driver** manages the destination (how to operate that specific device). E.g. the PCI subsystem handles "how do I talk to PCIe hardware"; your network driver handles "how do I operate this NIC."

All four converge on the same next step:

```
Device discovered (any bus)
        |
        v
Device registered — carries a generic struct device
        |
        v
        Driver matching
```

---

## 2. Phase A — Matching and probe()

**Matching** = "does any registered driver claim it can handle this device?" — checked via Vendor/Device ID (PCI/USB) or `compatible` string (`of_match_table`, Device Tree).

```
Device                              Driver
 compatible = "vendor,mydevice"  →   of_match_table: { .compatible = "vendor,mydevice" }
                    \               /
                     v             v
                    MATCH → probe() called
```

Order doesn't matter — device can appear before or after the driver is loaded; whichever comes second triggers the match. **No match = no `probe()`**, and the device just sits unbound (device existing ≠ device having a driver).

**`probe()` is not "search for hardware."** It's the driver's onboarding callback: "Linux matched you to this device — now initialize it." Typical work inside `probe(pdev)`:

1. Get resources (from `pdev` — Device Tree info, memory regions, IRQ)
2. Map registers (MMIO)
3. Request IRQ
4. Allocate a per-device driver struct (so multiple instances don't collide)
5. Initialize hardware, locks, buffers
6. Register with whichever kernel interface fits (char device, netdev, block, ALSA, DRM, etc.)

```c
struct my_device {
    void __iomem *regs;
    int irq;
    struct cdev cdev;
    struct mutex lock;
};
```
Each matched device instance gets its own struct — `probe()` may run once per device (Device 0 → `probe()`, Device 1 → `probe()`, ...).

On teardown, `remove()` does the mirror image: stop hardware, free IRQ/DMA, unmap registers, remove cdev, destroy device, release resources.

---

## 3. Phase B — where the interface diverges by driver type

`probe()` succeeding is the fork point. What you register with next depends on what kind of driver this is — **this is the only part of the whole flow that isn't generic**:

| Driver type | Registers as | Userspace sees |
|---|---|---|
| Character driver | `cdev` + `file_operations` | `/dev/foo`, open/read/write/ioctl |
| Block driver | `blk-mq` queue, `gendisk` | `/dev/sda`, block layer, bio/request |
| Network driver | `net_device` + `netdev_ops` | `eth0`, socket API — no `/dev` node at all |
| Sound (ALSA) | ALSA card/PCM device | `/dev/snd/*` |
| Graphics (DRM) | `drm_device` + KMS/GEM | `/dev/dri/card0` |
| Input | `input_dev` | `/dev/input/event*` |

Everything before this fork (discovery → matching → resource acquisition in `probe()`) is identical in concept across all of these. Sections 4–6 walk the three most commonly asked-about branches — character, block, network — in the same numbered style.

---

## 4. Phase B — Character device: from probe() to /dev/mydevice

One continuous sequence, numbered:

```
                         KERNEL
                           |
   1. Reserve a device number
      major identifies the driver, minor identifies the instance
                           ↓
                alloc_chrdev_region()  →  240 : 0  (major : minor)
                           |
   2. Initialize struct cdev, bind it to file_operations
                           ↓
                       cdev_init(&my_cdev, &my_fops)
                           |
   3. Tell the kernel "240:0 is handled by my_cdev"
                           ↓
                       cdev_add(&my_cdev, dev, 1)
                           |
   4. Create a logical class (userspace-visible grouping)
                           ↓
                    class_create()  →  /sys/class/mydevice/
                           |
   5. Create the device-model entry tied to 240:0
                           ↓
                    device_create()
                           |
   6. Kernel emits a uevent; devtmpfs/udev create the node
                           ↓
                    /dev/mydevice
                           |
   7. Application opens the path
                           ↓
                      open("/dev/mydevice")
                           |
                           ↓
                          VFS  →  resolves 240:0  →  my_cdev  →  my_fops
                           |
                           ↓
                       my_open() / my_read() / my_write() / my_ioctl()
                           |
                           ↓
                         DRIVER  →  registers / DMA / IRQ  →  HARDWARE
```

Key distinctions worth remembering explicitly:
- `cdev_add()` ≠ "create `/dev/mydevice`" — that's several layers further down (class → device_create → uevent → udev).
- `/dev/mydevice` is not a normal file; `ls -l` shows `c` (character device) and `240, 0` (major, minor) instead of a size.
- Major/minor is a device-number identity for routing I/O — it is **not** the hardware identity (Vendor ID / `compatible` string) used for matching in Phase A.

### struct device vs struct cdev vs struct file

```
Physical device → struct device (device model)
                        |
                     Driver
                        |
                    struct cdev (major/minor + file_operations)
                        |
                  /dev/mydevice
                        |
                     open() → struct file (one instance per open fd)
```

`file->private_data` is how a multi-instance driver (`/dev/mydevice0`, `/dev/mydevice1`, ...) tracks which specific device a given open `fd` refers to — set in `my_open()`, read back in `my_read()`/`my_write()`.

---

## 5. Phase B — Block device: from probe() to /dev/sda

Same fork point as Section 4, different registration. A block driver doesn't use `cdev`/`file_operations` at all — it registers a request queue and a disk object instead.

```
                         KERNEL (post-probe)
                           |
   1. Allocate a disk object
                           ↓
                  blk_alloc_disk() / alloc_disk()
                           |
   2. Set up the request queue (blk-mq)
      — how the driver receives I/O requests (reads/writes)
                           ↓
             blk_mq_alloc_tag_set() + blk_mq_init_queue()
                           |
   3. Attach block_device_operations
      (open/release/ioctl — NOT read/write; I/O goes via bios, not fops)
                           ↓
                    disk->fops = &my_block_fops
                           |
   4. Set device name, capacity, and register with the kernel
                           ↓
                    add_disk()
                           |
   5. Kernel creates the device-model entry + uevent → udev
                           ↓
                    /dev/mydisk  (or /dev/nvme0n1, /dev/sda, ...)
                           |
   6. Application issues I/O
                           ↓
              read()/write() via filesystem, or raw block I/O
                           |
                           ↓
                   Block layer builds a struct bio
                           |
                           ↓
              blk-mq dispatches it as a struct request
                           |
                           ↓
                  driver's queue_rq() callback
                           |
                           ↓
                DRIVER → DMA transfer, NVMe/SCSI command, etc.
                           |
                           ↓
                       HARDWARE (disk/SSD/controller)
```

Key differences from character devices:
- No `open()`/`read()`/`write()` file_operations doing the actual data transfer — the **bio → request → queue_rq()** path carries the data. `block_device_operations` only covers control-plane calls (open, ioctl, media detection).
- I/O is asynchronous and queued/batched (`blk-mq`), not a single synchronous syscall per operation — this is what enables request merging, I/O scheduling, and multi-queue parallelism across CPUs.
- Still gets a device node under `/dev`, still goes through the same `class`/`device_create`/uevent/udev machinery as a char device once `add_disk()` runs — that part of Phase B *is* shared.

---

## 6. Phase B — Network device: from probe() to ethX (no /dev node)

The interface most different from the char-device model. A NIC driver never creates a `/dev` entry at all — the "device" userspace sees is a named network interface, accessed through sockets, not `open()`.

```
                         KERNEL (post-probe)
                           |
   1. Allocate a net_device struct
                           ↓
                    alloc_etherdev() / alloc_netdev()
                           |
   2. Attach net_device_ops
      (open, stop, start_xmit, set_mac_address, ...)
                           ↓
                    dev->netdev_ops = &my_netdev_ops
                           |
   3. Set MAC address, MTU, feature flags, IRQ/NAPI handlers
                           ↓
                    (still inside probe())
                           |
   4. Register the interface with the network stack
                           ↓
                    register_netdev()
                           |
   5. Kernel assigns a name (eth0, enp3s0, ...) — no /dev node,
      no major/minor, no udev device file
                           ↓
                    Interface visible to `ip link` / userspace
                           |
   6. Interface brought up
                           ↓
              ifconfig eth0 up  /  ip link set eth0 up
                           |
                           ↓
                  my_netdev_ops->ndo_open() called
                           |
                           ↓
                Driver enables IRQ, starts DMA rings
                           |
   7. Application sends/receives data
                           ↓
              socket() → send()/recv() (no /dev, no fd on a char device)
                           |
                           ↓
                     Network stack (TCP/IP)
                           |
                           ↓
              ndo_start_xmit() (TX)  /  NAPI poll + IRQ (RX)
                           |
                           ↓
                DRIVER → DMA descriptor rings
                           |
                           ↓
                       HARDWARE (NIC)
```

Key differences from character/block devices:
- **No device node at all.** No `alloc_chrdev_region()`, no `cdev`, no `class_create()`/`device_create()`, no udev involvement. The "handle" is the interface name (`eth0`), managed entirely by the network stack (`rtnetlink`/`ip link`).
- Userspace never calls `open("/dev/...")` on it — it calls `socket()` and lets the network stack route packets to the right `net_device`.
- RX is typically interrupt + NAPI poll driven (mitigates interrupt storms under load) rather than a blocking `read()` call.
- `ndo_open()`/`ndo_stop()` are the closest analogue to `file_operations` open/release, but they're triggered by `ip link set up/down`, not by an application opening a path.

---

## 7. Study path for interview prep

```
L1  User/kernel boundary — syscalls, VFS
L2  Device model — struct device, bus, driver binding/matching
L3  Discovery — PCI enumeration, Device Tree, ACPI, platform devices
L4  Driver binding — platform_driver / pci_driver / usb_driver, probe()/remove()
L5  Character devices — major/minor, dev_t, cdev, file_operations, class, /dev, udev
L6  Runtime I/O — open, read, write, ioctl, poll, mmap
L7  Hardware access — MMIO, ioremap, readl/writel, IRQ, DMA
L8  Concurrency — mutex, spinlock, wait queue, completion, atomics
L9  Advanced — PCIe, IOMMU, power management, sysfs, debugfs, ftrace, lockdep
L10 Storage — block layer, bio, request, blk-mq, SCSI, NVMe
```

Given your storage + device-driver background, the throughline worth being able to draw on a whiteboard cold is:

```
Application → syscall → VFS → char/block interface → driver
   → Linux device model → bus → PCIe → DMA/MMIO/IRQ → hardware
```

...and being able to explain **why** character, block, and network drivers diverge after `probe()` (Section 3 above) even though everything before that point — discovery, matching, resource acquisition — is the same. That divergence (Sections 4–6) is a favorite senior-level interview probe precisely because it separates people who memorized one API from people who understand the layered architecture.
