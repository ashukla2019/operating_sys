# Linux Device Driver Mental Model

## 1. First build the mental model

A Linux device driver has two sides:

```
                    LINUX KERNEL
                         |
          +--------------+--------------+
          |                             |
          |  DEVICE MODEL               |
          |                             |
          |  "What hardware exists?"    |
          |  "Which driver owns it?"     |
          |                             |
          +--------------+--------------+
                         |
                         v
                    YOUR DRIVER
                         |
                         |
              Character Device Interface
                         |
                         v
                    /dev/mydevice
                         |
                         v
                    USER PROGRAM
```

There are therefore two related but different mechanisms:

### Mechanism A — Device ↔ Driver matching

This answers:

> "Which driver should handle this hardware?"

This is where:

- Device Tree
- ACPI
- PCI enumeration
- platform devices
- `probe()`
- `remove()`
- driver registration

come in.

### Mechanism B — User ↔ Driver interface

This answers:

> "How does my application communicate with the driver?"

This is where:

- major number
- minor number
- `dev_t`
- `cdev`
- `/dev/mydevice`
- `file_operations`
- `open()`
- `read()`
- `write()`
- `ioctl()`

come in.

**Do not mix these two mechanisms.** That is probably the biggest thing you need to understand first.

---

## 2. Start from the physical hardware

Suppose your machine has a hardware device:

```
             Physical Hardware
                    |
                    |
             +------+------+
             |             |
           Device        Device
          registers       IRQ
             |
             |
          PCIe / SoC / I2C / SPI
```

Linux starts by discovering what devices exist. The exact discovery mechanism depends on the hardware. For example:

```
PCI device       -> PCI subsystem discovers it
USB device       -> USB subsystem discovers it
I2C device       -> I2C subsystem discovers it
SPI device       -> SPI subsystem discovers it
SoC device       -> Device Tree / ACPI + platform bus
```

This is the first major concept:

> The driver normally does NOT magically discover arbitrary hardware by itself. A bus/subsystem usually discovers or describes the device first.

---

## 3. How does Linux know a device exists?

There are several possibilities.

### Case 1: PCI

Suppose you have:

```
NVMe Controller
      |
      v
     PCIe
```

PCI hardware has configuration information including identifiers such as:

- Vendor ID
- Device ID

Linux scans the PCI bus.

```
PCI Bus
   |
   +-- Device 0
   |
   +-- Device 1
   |
   +-- Device 2
         |
         +-- Vendor ID
         +-- Device ID
         +-- BARs
         +-- IRQ information
```

Linux creates a kernel representation: `struct pci_dev`

Now Linux knows: "There is a PCI device with this identity."

---

## 4. Case 2: Device Tree

On many ARM/embedded systems, hardware cannot simply be discovered like PCI. Instead, firmware provides a Device Tree describing hardware. For example, conceptually:

```
soc {
    mydevice@10000000 {
        compatible = "vendor,mydevice";
        reg = <0x10000000 0x1000>;
        interrupts = <42>;
    };
};
```

This tells Linux:

- There is a device
- `compatible = "vendor,mydevice"`
- registers = 0x10000000
- interrupt = 42

Linux parses this and creates a device representation.

```
Device Tree
     |
     v
Linux parses DT
     |
     v
struct device / platform_device
     |
     v
Platform bus
```

So now Linux knows: "There is a device called mydevice and its hardware resources are here."

---

## 5. Case 3: USB

USB is different again. When you plug in a USB device:

```
USB Device
    |
    v
USB Host Controller
    |
    v
USB subsystem
    |
    v
USB device discovered
```

The USB device reports identifiers, for example:

- Vendor ID
- Product ID

Linux creates something like `struct usb_device` and later attempts to match a driver.

---

## 6. So what is a "device" in Linux?

This is important. A Linux device is not simply `/dev/mydevice`.

The kernel has its own device model. At the heart of it is: `struct device`

You can think of it as the kernel's representation of: "This particular device exists."

It can contain/refer to things such as:

```
struct device
 |
 +-- device name
 +-- parent
 +-- bus
 +-- driver
 +-- device tree information
 +-- power information
 +-- DMA information
 +-- device class
 +-- device number
 +-- ...
```

The exact fields and relationships are more complex, but this mental model is enough initially.

---

## 7. Now the important question: how does the driver get selected?

Suppose Linux discovers:

```
Device:
compatible = "vendor,mydevice"
```

You have a driver:

```c
static const struct of_device_id my_ids[] = {
    {
        .compatible = "vendor,mydevice",
    },
    {}
};
```

and:

```c
static struct platform_driver my_driver = {
    .probe = my_probe,
    .remove = my_remove,

    .driver = {
        .name = "mydriver",
        .of_match_table = my_ids,
    },
};
```

When your driver is registered:

```c
platform_driver_register(&my_driver);
```

Linux has:

```
                  Platform Bus
                       |
             +---------+---------+
             |                   |
             v                   v
         Device A             Device B
       "mydevice"             "other"

             |
             | matching
             v

        my_driver
             |
             v
          probe()
```

This is the key.

---

## 8. What exactly is "matching"?

Matching means: Does this driver claim that it can handle this device?

For Device Tree, one common mechanism is:

Device: `compatible = "vendor,mydevice"`

Driver: `.of_match_table = my_ids` with `{ .compatible = "vendor,mydevice" }`

```
Device
   |
   | compatible = vendor,mydevice
   |
   +--------------------+
                        |
                        v
                   Driver table
                        |
                        | match
                        v
                    my_driver
```

Linux says: "This driver has a matching entry. Therefore this driver can handle this device." Then `probe()` is called.

---

## 9. probe() does NOT mean "search for devices"

This is a very important correction to a common misunderstanding. Many beginners think:

```
probe()
   |
   +--> find hardware
```

Not exactly. The typical sequence is:

```
Device discovered
       |
       v
Device registered
       |
       v
Driver registered
       |
       v
Matching
       |
       v
probe()
```

So `probe()` essentially means: "Linux has matched this driver with this particular device. Initialize this device."

---

## 10. Think of probe() as onboarding

Imagine Linux says: "Hey mydriver, I found a device that you said you support." Then `my_probe(...)` runs.

Your `probe()` might:

1. Get device resources
2. Map registers
3. Allocate driver data
4. Request IRQ
5. Initialize hardware
6. Initialize locks
7. Initialize buffers
8. Register character device
9. Create `/dev/mydevice`
10. Make device ready

```
                probe()
                   |
       +-----------+-----------+
       |           |           |
       v           v           v
    MMIO          IRQ       memory
       |           |           |
       +-----------+-----------+
                   |
                   v
              initialize
                   |
                   v
              cdev setup
                   |
                   v
             /dev/mydevice
```

---

## 11. Now we reach major/minor

This is where the second mechanism begins. You have successfully matched hardware to your driver. Now you want `/dev/mydevice` so that user space can communicate with it. Linux needs a way to identify the device node. That is where **major** and **minor** come in.

---

## 12. What is a major number?

Think: `major number = which driver/subsystem handles this device node?`

Historically, the major number identifies the driver associated with the device node. For example:

```
/dev/mydevice

major = 240
minor = 0
```

The kernel can associate:

```
major 240
       |
       v
character-device registration
       |
       v
your driver
```

---

## 13. What is a minor number?

Minor identifies the particular device/instance handled by that driver. Suppose your driver handles three devices:

```
/dev/mydevice0
/dev/mydevice1
/dev/mydevice2
```

You might have:

```
              major = 240

/dev/mydevice0 -> minor 0
/dev/mydevice1 -> minor 1
/dev/mydevice2 -> minor 2
```

Therefore: Major -> driver/category, Minor -> particular instance. This is the basic mental model.

---

## 14. But don't confuse major/minor with hardware identification

This is very important. Suppose:

```
PCI Device
Vendor ID = 1234
Device ID = 5678
```

and:

```
/dev/mydevice
major = 240
minor = 0
```

These are not the same identification system.

Hardware matching (Vendor ID, Device ID, PCI class, Device Tree compatible, ACPI IDs, USB IDs, ...) is used to answer: "Which driver should control this hardware?"

Major/minor is used primarily to identify a character/block device node for kernel I/O routing.

So: `Hardware matching != major/minor`

This distinction will eliminate a lot of confusion.

---

## 15. Now connect everything

Suppose we have:

```
Physical Device
      |
      v
Device Tree
      |
      v
Linux discovers device
      |
      v
platform_device
      |
      v
Platform bus
      |
      | matching
      v
platform_driver
      |
      v
probe()
      |
      +---- initialize hardware
      |
      +---- allocate driver context
      |
      +---- register character device
                     |
                     v
                 major/minor
                     |
                     v
                   cdev
                     |
                     v
              file_operations
                     |
                     v
                /dev/mydevice
```

Now the application can do `open("/dev/mydevice", ...)` and the path becomes:

```
Application
     |
     v
system call
     |
     v
VFS
     |
     v
/dev/mydevice
     |
     v
major/minor
     |
     v
cdev
     |
     v
file_operations
     |
     v
my_open()
```

This is the complete connection.

---

## 16. There are actually TWO IDs involved

This is probably the single most useful thing to remember.

**ID #1 — Hardware identity.** Used for device ↔ driver matching. Examples:
- PCI: Vendor ID + Device ID
- USB: Vendor ID + Product ID
- Device Tree: `compatible = "vendor,device"`
- ACPI: ACPI/HID IDs

**ID #2 — Device number.** Used for `/dev` node ↔ character/block device. Examples: major = 240, minor = 0

```
           HARDWARE
               |
               | hardware ID
               v
       Device/Driver Matching
               |
               v
             Driver
               |
               | creates character device
               v
          major/minor
               |
               v
        /dev/mydevice
               |
               v
          User program
```

---

## 17. Now let's understand cdev

Once your driver has a major/minor number, Linux needs to associate that character-device number with your driver's operations. That's where `struct cdev` comes in.

```
             dev_t
          major + minor
               |
               v
             cdev
               |
               v
       file_operations
               |
       +-------+-------+
       |       |       |
      open    read    write
```

You initialize:

```c
cdev_init(&my_cdev, &my_fops);
```

and register:

```c
cdev_add(&my_cdev, dev, 1);
```

Now Linux knows:

```
device number
      |
      v
    cdev
      |
      v
file_operations
      |
      +--> my_open()
      +--> my_read()
      +--> my_write()
```

---

## 18. Where does /dev/mydevice come from?

This is another common confusion. `cdev_add()` does not simply mean "create /dev/mydevice." There are several layers:

```
alloc_chrdev_region()
       |
       v
   major/minor
       |
       v
    cdev_add()
       |
       v
kernel character-device registration
       |
       v
class_create()
       |
       v
device_create()
       |
       v
device model / uevent
       |
       v
udev
       |
       v
/dev/mydevice
```

So `cdev` and `/dev/mydevice` are related but not the same thing.

---

## 19. What does udev do?

udev runs in user space and reacts to kernel device events. When the kernel creates/registers a device, it can generate a uevent.

```
Kernel
   |
   | uevent
   v
udev
   |
   | creates device node
   v
/dev/mydevice
```

This is why modern Linux systems can automatically populate `/dev`. Don't think "driver directly creates a normal file" — instead think: Driver/device model → kernel device event → udev → `/dev` node.

---

## 20. Is /dev/mydevice a normal file?

No. It looks like a file from the application's perspective (`open()`, `read()`, `write()`, `close()`) but it is a device node. For example:

```
ls -l /dev/mydevice
```

You may see something like:

```
crw-rw---- 1 root ... 240, 0 ... /dev/mydevice
```

The first character `c` means character device. `240, 0` are major = 240, minor = 0. This is a fantastic command to use when learning drivers.

---

## 21. Now let's trace open() in detail

Application:

```c
fd = open("/dev/mydevice", O_RDWR);
```

**Step 1** — Application asks kernel: `open("/dev/mydevice")`
**Step 2** — VFS resolves the pathname.
**Step 3** — It discovers: this is a character device, major = 240, minor = 0
**Step 4** — Kernel's character-device machinery uses that device number to find the registered character device.
**Step 5** — That leads to `cdev`
**Step 6** — cdev has/associates with `file_operations`
**Step 7** — VFS calls `my_open()`

```
open("/dev/mydevice")
        |
        v
      VFS
        |
        v
 character device node
        |
        v
    major/minor
        |
        v
       cdev
        |
        v
file_operations
        |
        v
     my_open()
```

---

## 22. Where does probe() fit relative to open()?

This is critical. `probe()` normally happens before user space opens the device.

```
BOOT / DEVICE DISCOVERY
        |
        v
device discovered
        |
        v
driver matched
        |
        v
probe()
        |
        v
driver initialized
        |
        v
/dev/mydevice available
        |
        v
USER APPLICATION
        |
        v
open()
        |
        v
my_open()
```

Therefore: `probe() != open()`. `probe()` is device initialization/binding. `open()` is a user process opening the already-bound device interface.

---

## 23. probe() may happen multiple times

Suppose your driver supports multiple devices: Device 0, Device 1, Device 2. Your driver has one `probe()` function: `my_probe(...)`. Linux can call it separately:

```
Device 0 ----> my_probe()
Device 1 ----> my_probe()
Device 2 ----> my_probe()
```

Each call initializes one device instance. This is why you often allocate a device-specific structure:

```c
struct my_device {
    void __iomem *regs;
    int irq;
    struct cdev cdev;
    struct mutex lock;
    ...
};
```

Then:

```
Device 0
    |
    +--> struct my_device #0

Device 1
    |
    +--> struct my_device #1
```

---

## 24. How does probe() know which device it is initializing?

Because Linux passes information about the matched device. For a platform driver, you commonly see:

```c
static int my_probe(struct platform_device *pdev)
```

`pdev` represents the particular platform device. From it, you can obtain: resources, memory regions, IRQ, Device Tree information, device structure, DMA information, ...

```
Device Tree
    |
    v
platform_device
    |
    v
probe(pdev)
    |
    v
specific hardware instance
```

---

## 25. What if the driver is loaded after the device?

This is important. Suppose the device already exists and later you run `insmod mydriver.ko`. When the driver registers via `platform_driver_register()`, Linux checks existing devices for matches. If it finds a device <-> driver match, then `probe()` is called.

So it isn't necessarily "device first then driver" or "driver first then device" — either order can work.

```
Device appears
      |
      |------------------+
      |                  |
      v                  |
Driver registered        |
      |                  |
      +--------+---------+
               |
             match
               |
               v
             probe()
```

This is a very important Linux device-model concept.

---

## 26. What happens if there is no matching driver?

Suppose Linux discovers `compatible = "vendor,unknown"` but no registered driver supports `"vendor,unknown"`. Then:

```
Device
   |
   v
No matching driver
   |
   v
No probe()
```

The device can exist in the kernel without having a bound driver. Another key distinction: `Device exists != Device has a driver`

---

## 27. What happens when driver matching succeeds?

```
                 Device
                   |
                   |
                   v
             Device ID
                   |
                   v
             Match process
                   |
            +------+------+
            |             |
          Match         No match
            |             |
            v             v
         probe()       no probe()
            |
            v
      Driver bound
            |
            v
     Device initialized
```

---

## 28. What exactly is a bus?

Linux has buses such as PCI, USB, I2C, SPI, platform. A bus connects Devices ↕ Drivers:

```
                    BUS
                     |
          +----------+----------+
          |                     |
       Devices               Drivers
          |                     |
          +----------+----------+
                     |
                  matching
                     |
                     v
                  probe()
```

The bus often provides the rules for matching.

---

## 29. Platform bus

For SoC/embedded hardware:

```
CPU / SoC
   |
   +-- UART
   +-- GPIO
   +-- SPI
   +-- I2C
   +-- custom controller
```

Device Tree describes those devices. Linux creates platform devices. Your driver registers `struct platform_driver`, then matching occurs.

```
Device Tree
    |
    v
platform_device
    |
    v
platform bus
    |
    | match
    v
platform_driver
    |
    v
probe()
```

---

## 30. PCI is similar conceptually

```
PCI hardware
    |
    v
PCI subsystem enumerates
    |
    v
struct pci_dev
    |
    v
PCI driver matching
    |
    v
probe()
```

The difference is how the device is discovered and what information is available.

---

## 31. The complete architecture

Here is the diagram worth memorizing:

```
                           PHYSICAL HARDWARE
                                  |
                 +----------------+----------------+
                 |                |                |
                PCI              USB          SoC/Embedded
                 |                |                |
                 v                v                v
              PCI bus          USB bus       Device Tree/ACPI
                 |                |                |
                 v                v                v
             pci_dev          usb_device     platform_device
                 |                |                |
                 +----------------+----------------+
                                  |
                                  v
                             BUS MATCHING
                                  |
                                  v
                              DRIVER
                                  |
                                  v
                              probe()
                                  |
                    +-------------+-------------+
                    |             |             |
                    v             v             v
                  MMIO           IRQ          DMA
                    |             |             |
                    +-------------+-------------+
                                  |
                                  v
                        DEVICE INITIALIZATION
                                  |
                                  v
                           CHARACTER DEVICE
                                  |
                                  v
                      alloc_chrdev_region()
                                  |
                                  v
                             major/minor
                                  |
                                  v
                              cdev_add()
                                  |
                                  v
                           file_operations
                                  |
                                  v
                          device_create()
                                  |
                                  v
                               uevent
                                  |
                                  v
                                udev
                                  |
                                  v
                           /dev/mydevice
                                  |
                                  v
                           USER APPLICATION
                                  |
                     +------------+------------+
                     |            |            |
                   open()       read()       write()
                     |            |            |
                     +------------+------------+
                                  |
                                  v
                                 VFS
                                  |
                                  v
                              cdev / fops
                                  |
                                  v
                            DRIVER CALLBACKS
                                  |
                                  v
                              HARDWARE
```

---

## 32. One subtle but very important point

You might ask: "Does major number identify my physical hardware?" **No.**

```
                 Physical Hardware
                       |
                 Device Tree
                       |
                compatible ID
                       |
                       v
                 Driver matching
                       |
                       v
                    Driver
                       |
          +------------+------------+
          |                         |
      device 0                  device 1
          |                         |
      minor 0                   minor 1
          |                         |
          +------------+------------+
                       |
                  major = 240
```

The hardware identity and character-device identity are different layers.

---

## 33. Another subtle point: /dev is not the device

```
Physical device
      |
      v
Kernel device object
      |
      v
Driver
      |
      v
Character device
      |
      v
/dev/mydevice
```

`/dev/mydevice` is merely the user-visible handle. The actual hardware might be a PCIe NVMe controller, or an I2C sensor, or a UART controller.

---

## 34. Why do we need both device model and character-device model?

Because they solve different problems.

**Device model** answers: What hardware exists? Who controls it? What bus is it on? What resources does it have? What is its parent? What is its driver?

**Character-device subsystem** answers: How does user space access it? What major/minor does it have? Which `file_operations` should run?

```
DEVICE MODEL
     |
     | binds hardware to driver
     v
DRIVER
     |
     | exposes interface
     v
CHAR DEVICE
     |
     v
/dev/mydevice
     |
     v
APPLICATION
```

---

## 35. Now connect struct device, struct cdev, and struct file

These are three things you should distinguish.

**`struct device`** — Represents the device in Linux's device model.

```
hardware
   |
   v
struct device
```

**`struct cdev`** — Represents the character device registration.

```
major/minor
    |
    v
struct cdev
    |
    v
file_operations
```

**`struct file`** — Represents an open instance from a process.

```
application
    |
    | open()
    v
struct file
```

So:

```
Physical device
      |
      v
struct device
      |
      v
Driver
      |
      v
struct cdev
      |
      v
/dev/mydevice
      |
      v
open()
      |
      v
struct file
```

This is a very useful mental model.

---

## 36. file->private_data

Now the connection becomes even more powerful. Suppose `/dev/mydevice0` is opened. Your `open()` can associate `struct my_device` with `file->private_data`. Then:

```
/dev/mydevice0
       |
       v
     open()
       |
       v
struct file
       |
       +---- private_data
                 |
                 v
        struct my_device #0
                 |
                 v
              hardware
```

Later, `read(file, ...)` can recover:

```c
struct my_device *dev = file->private_data;
```

This is how the driver knows: "Which actual device instance is this file referring to?"

---

## 37. What you should learn next

Since you're specifically preparing for senior Linux/device-driver interviews, don't stop at the basic character-device API. Build the knowledge in these layers:

**LEVEL 1 — Linux user/kernel space**
- System calls
- VFS

**LEVEL 2 — Device model**
- `struct device`
- bus
- device
- driver
- binding
- matching

**LEVEL 3 — Device discovery**
- PCI enumeration
- Device Tree
- ACPI
- platform devices

**LEVEL 4 — Driver binding**
- `platform_driver`
- `pci_driver`
- `usb_driver`
- `probe()`
- `remove()`

**LEVEL 5 — Character devices**
- major/minor
- `dev_t`
- `cdev`
- `file_operations`
- class
- device
- `/dev`
- udev

**LEVEL 6 — Runtime I/O**
- open
- read
- write
- ioctl
- poll
- mmap

**LEVEL 7 — Hardware**
- MMIO
- `ioremap`
- `readl`/`writel`
- IRQ
- DMA

**LEVEL 8 — Concurrency**
- mutex
- spinlock
- wait queue
- completion
- atomic

**LEVEL 9 — Advanced**
- DMA
- PCIe
- IOMMU
- power management
- runtime PM
- device tree
- sysfs
- debugfs
- ftrace
- lockdep

**LEVEL 10 — Storage**
- Block layer
- bio
- request
- blk-mq
- SCSI
- NVMe
- PCIe
- DMA

For storage and device-driver experience specifically, the final goal should be to connect:

```
Application
    ↓
System Call
    ↓
VFS
    ↓
Character / Block Interface
    ↓
Driver
    ↓
Linux Device Model
    ↓
Bus
    ↓
PCIe
    ↓
DMA / MMIO / IRQ
    ↓
Hardware
```

and then separately understand why **character driver** and **storage/block driver** take different paths through the kernel.

The next logical step is to take one tiny platform character driver and trace every object and callback:

```
Device Tree
   ↓
platform_device
   ↓
platform_driver_register()
   ↓
matching
   ↓
probe()
   ↓
devm_* resources
   ↓
alloc_chrdev_region()
   ↓
cdev_init()
   ↓
cdev_add()
   ↓
class_create()
   ↓
device_create()
   ↓
udev
   ↓
/dev/mydevice
   ↓
open()
   ↓
struct file
   ↓
file->private_data
   ↓
read/write/ioctl
   ↓
MMIO/IRQ
   ↓
hardware
```
