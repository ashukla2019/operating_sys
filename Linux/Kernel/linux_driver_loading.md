# Linux Driver Loading (Static vs Dynamic)

---

# Driver Loading Methods

Linux supports two ways to load a device driver:

1. **Static Loading (Built into Kernel)**
2. **Dynamic Loading (Loadable Kernel Module - LKM)**

```
                Linux Kernel
                     │
        ┌────────────┴────────────┐
        │                         │
 Built-in Driver             Loadable Module
 (Static)                       (.ko)
```

---

# 1. Static Driver Loading

The driver is compiled directly into the Linux kernel (`vmlinuz`).

```
Application
      │
System Call
      │
Linux Kernel
      │
Built-in Driver
      │
Hardware
```

## Build Statically

Run:

```bash
make menuconfig
```

Select:

```
[*] Driver Name
```

Build and install the kernel:

```bash
make
make modules
make modules_install
make install

---------|---------|--------|
| `make` | Builds the kernel image and built-in drivers | `vmlinux`, `bzImage`, built-in objects |
| `make modules` | Compiles loadable kernel modules | `.ko` files |
| `make modules_install` | Installs `.ko` files into `/lib/modules/<kernel-version>/` | Installed modules + dependency files |
| `make install` | Installs the kernel into `/boot/` and updates boot configuration | `vmlinuz`, `System.map`, `config`, often `initramfs` |

```

The driver is now permanently part of the kernel.

### Advantages

- Available immediately during boot
- Required for boot-critical hardware
- Faster startup (no separate module loading)
- Cannot be accidentally removed

### Disadvantages

- Increases kernel size
- Cannot unload without reboot
- Kernel rebuild required after driver changes

### Examples

- CPU Scheduler
- Memory Management
- ext4 filesystem (Root FS)
- SATA/NVMe Controller
- MMC Driver
- Early Console Driver

---

# 2. Dynamic Driver Loading

The driver is compiled as a **Loadable Kernel Module (LKM)**.

Output:

```
driver.ko
```

```
Application
      │
System Call
      │
Linux Kernel
      │
driver.ko
      │
Hardware
```

Modules can be loaded or removed while Linux is running.

---

# Build as Module

Select:

```
<M> Driver Name
```

Compile:

```bash
make modules
```

---

# Loading a Module

## insmod

```bash
sudo insmod driver.ko
```

- Loads a specific module
- Does NOT load dependencies
- Requires full module path

---

## modprobe

```bash
sudo modprobe driver_name
```

- Automatically loads dependencies
- Searches `/lib/modules/<kernel-version>/`
- Uses module dependency database

Preferred in production systems.

---

# Checking Loaded Modules

```bash
lsmod
```

Example:

```
Module          Size    Used by
e1000          245760      1
usb_storage     69632      0
```

---

# Module Information

```bash
modinfo driver_name
```

Displays:

- Version
- Author
- Description
- License
- Dependencies
- Supported devices

---

# Removing a Module

Using rmmod

```bash
sudo rmmod driver_name
```

Using modprobe

```bash
sudo modprobe -r driver_name
```

---

# Module Lifecycle

When loaded:

```
insmod/modprobe
        │
module_init()
        │
Register Driver
        │
Allocate Resources
        │
Probe Device
        │
Driver Ready
```

When removed:

```
rmmod
     │
module_exit()
     │
Stop Device
     │
Free Resources
     │
Unregister Driver
```

Example:

```c
module_init(my_driver_init);
module_exit(my_driver_exit);
```

---

# How Linux Automatically Loads Drivers

```
Hardware Connected
        │
Kernel detects device
        │
Generate uevent
        │
udev receives event
        │
modprobe
        │
Driver Loaded
        │
probe() called
```

Example:

```
USB Mouse Inserted
        │
USB Core detects device
        │
Kernel generates uevent
        │
udev runs modprobe usbhid
        │
usbhid.ko loaded
        │
probe() executes
        │
Mouse Ready
```

---

# Boot-Time Driver Loading

```
Power ON
    │
Bootloader
    │
Kernel Starts
    │
Initialize Built-in Drivers
    │
Mount Root Filesystem
    │
Start init/systemd
    │
Start udev
    │
Load Remaining Modules
    │
Userspace Starts
```

---

# Static vs Dynamic

| Feature | Static Driver | Dynamic Module |
|----------|---------------|----------------|
| Built into Kernel | Yes | No |
| Separate `.ko` File | No | Yes |
| Can Load After Boot | No | Yes |
| Can Unload | No | Yes |
| Requires Reboot After Update | Yes | No |
| Memory Usage | Always Loaded | Loaded On Demand |
| Kernel Rebuild Needed | Yes | No |
| Used For | Boot-Critical Drivers | Most Device Drivers |

---

# Driver Registration Flow

```
Driver Loaded
      │
module_init()
      │
Register Driver
      │
Kernel Driver List
      │
Matching Device Found
      │
probe()
      │
Initialize Hardware
      │
Device Ready
```

---

# Driver Removal Flow

```
rmmod
   │
module_exit()
   │
remove()
   │
Stop Hardware
   │
Free Memory
   │
Unregister Driver
```

---

# Common Commands

```bash
# Load module
insmod driver.ko

# Load with dependencies
modprobe driver_name

# Remove module
rmmod driver_name

# Remove with dependencies
modprobe -r driver_name

# Show loaded modules
lsmod

# Module details
modinfo driver_name

# Kernel log
dmesg | tail -50

# View loaded modules
cat /proc/modules
```

---

# Interview Points

## Static Driver

- Compiled into kernel image.
- Loaded during kernel boot.
- Cannot be unloaded.
- Suitable for root filesystem and storage drivers.

---

## Dynamic Driver

- Stored as `.ko` file.
- Loaded using `insmod` or `modprobe`.
- Can be unloaded using `rmmod`.
- Saves memory by loading only when needed.

---

## insmod vs modprobe

| insmod | modprobe |
|--------|----------|
| Loads one module | Loads module + dependencies |
| Needs full path | Searches `/lib/modules` |
| No dependency handling | Automatic dependency handling |
| Used for testing | Used in production |

---

# Driver Loading Sequence (Most Important Interview Flow)

```
Driver Compiled
        │
Static OR Dynamic
        │
Kernel Loads Driver
        │
module_init()
        │
Register Driver
        │
Kernel Matches Device
        │
probe()
        │
Allocate Resources
        │
Map Registers
        │
Request IRQ
        │
Initialize Hardware
        │
Device Ready
        │
Application Uses Device
        │
Driver Removed (Optional)
        │
remove()
        │
module_exit()
```

---

# Senior Interview Questions

1. Explain static vs dynamic driver loading.
2. Why are root filesystem drivers usually built into the kernel?
3. What is a Loadable Kernel Module (LKM)?
4. Difference between `insmod` and `modprobe`.
5. What does `module_init()` do?
6. What does `module_exit()` do?
7. How does Linux automatically load drivers using `udev`?
8. What happens internally after `modprobe` loads a driver?
9. Explain the sequence: **Driver Load → Registration → Device Match → probe()**.
10. Which commands are used to load, unload, inspect, and debug kernel modules?

-----------------
# Simple Linux Kernel Driver (Hello World Module)

This is the smallest Linux kernel driver used to understand **module loading and unloading**.

---

# hello.c

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Simple Hello Driver");

static int __init hello_init(void)
{
    printk(KERN_INFO "Hello Driver Loaded!\n");
    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO "Hello Driver Unloaded!\n");
}

module_init(hello_init);
module_exit(hello_exit);
```

---

# Makefile

```makefile
obj-m += hello.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
```

---

# Build the Driver

```bash
make
```

Output:

```
hello.ko
hello.o
hello.mod.c
```

---

# Load the Driver

```bash
sudo insmod hello.ko
```

or

```bash
sudo modprobe hello
```

---

# Verify It Loaded

```bash
lsmod | grep hello
```

Example:

```
hello 16384 0
```

---

# View Kernel Messages

```bash
dmesg | tail
```

Output:

```
Hello Driver Loaded!
```

---

# Remove the Driver

```bash
sudo rmmod hello
```

Check the log again:

```bash
dmesg | tail
```

Output:

```
Hello Driver Unloaded!
```

---

# Driver Execution Flow

```
hello.c
    │
    ▼
make
    │
    ▼
hello.ko
    │
    ▼
insmod hello.ko
    │
    ▼
module_init()
    │
    ▼
hello_init()
    │
    ▼
printk("Hello Driver Loaded")
    │
    ▼
Driver Active
    │
    ▼
rmmod hello
    │
    ▼
module_exit()
    │
    ▼
hello_exit()
    │
    ▼
printk("Hello Driver Unloaded")
```

---

# What Each Part Does

```c
MODULE_LICENSE("GPL");
```
- Declares the module license.
- `"GPL"` enables access to GPL-only kernel symbols and avoids a kernel taint warning.

```c
MODULE_AUTHOR("Your Name");
```
- Stores author information.

```c
MODULE_DESCRIPTION("Simple Hello Driver");
```
- Stores a description visible via `modinfo`.

```c
static int __init hello_init(void)
```
- Runs **once** when the module is loaded.
- Used for initialization (register devices, allocate memory, request IRQs, etc.).

```c
static void __exit hello_exit(void)
```
- Runs when the module is unloaded.
- Used for cleanup (free memory, unregister devices, release IRQs, etc.).

```c
module_init(hello_init);
```
- Registers `hello_init()` as the module's entry point.

```c
module_exit(hello_exit);
```
- Registers `hello_exit()` as the module's cleanup function.

```c
printk(KERN_INFO "Hello Driver Loaded!\n");
```
- Prints a message to the kernel log (view with `dmesg`).

---

# Interview Takeaway

This is a **kernel module**, not a real hardware driver. A real device driver extends this skeleton by:

1. Registering with a kernel subsystem (platform, PCI, USB, I2C, SPI, etc.).
2. Implementing `probe()` and `remove()`.
3. Initializing hardware (map registers, request IRQs, allocate resources).
4. Exposing interfaces to user space (character device, sysfs, netdev, etc.).