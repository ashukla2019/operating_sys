# Linux Device Driver — Consolidated Reference

Every driver type follows the same two-phase shape. Keep this split in your head above all else:

| Phase | Question it answers | Key players |
|---|---|---|
| **A. Discovery / matching** | "Which driver should handle this hardware?" | bus subsystem, Device Tree/ACPI, `probe()`/`remove()` |
| **B. Runtime interface** | "How does an application talk to the driver?" | major/minor, `cdev`, `file_operations`, `/dev/*` |

**Don't mix these two.** Matching identity (Vendor ID, `compatible` string) and device-number identity (major:minor) are unrelated systems that happen to both point at "your driver."

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

Everything before this fork (discovery → matching → resource acquisition in `probe()`) is identical in concept across all of these. The rest of this document follows the **character-device** path in detail, since that's the canonical one to know cold.

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

## 5. Study path for interview prep

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

...and being able to explain **why** a character driver and a block/storage driver diverge after `probe()` (Section 3 above) even though everything before that point is the same.
