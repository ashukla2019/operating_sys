# Chapter 14 – Linux Kernel Modules

---

# 1. What Is a Kernel Module?

A kernel module is code that can be dynamically loaded into the Linux kernel without rebuilding or rebooting the kernel.

Typical modules include:

```text
Device Drivers
Filesystem Drivers
Network Drivers
USB Drivers
Storage Drivers
Security Components
Kernel Extensions
```

Conceptually:

```text
                    Linux Kernel
                         |
        +----------------+----------------+
        |                |                |
   Built-in Code    Kernel Module    Kernel Module
                         |
                         v
                  Device Driver
```

---

# 2. Why Kernel Modules?

Without modules, adding a driver would generally require rebuilding/replacing the kernel image.

With modules:

```text
Kernel running
     |
     +-- Load driver module
     |
     +-- Driver becomes available
```

Advantages:

```text
Dynamic loading
Smaller base kernel
Easier driver development
Optional functionality
Device-specific support
```

---

# 3. Built-in vs Module

Kernel functionality can be:

```text
Built into kernel
        OR
Built as module
```

Conceptually:

```text
CONFIG_DRIVER=y
    ↓
Built into kernel

CONFIG_DRIVER=m
    ↓
Built as module
```

This distinction is extremely important.

---

# 4. Kernel Module File

Kernel modules are commonly stored as:

```text
/lib/modules/<kernel-version>/
```

Modern Linux modules generally have:

```text
.ko
```

extension.

Example:

```text
my_driver.ko
```

`.ko` means:

```text
Kernel Object
```

---

# 5. Basic Module Structure

A simple module contains:

```c
#include <linux/module.h>
#include <linux/kernel.h>

static int __init my_init(void)
{
    pr_info("Module loaded\n");
    return 0;
}

static void __exit my_exit(void)
{
    pr_info("Module unloaded\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Author");
MODULE_DESCRIPTION("Example kernel module");
```

The important pieces are:

```text
module_init()
module_exit()
MODULE_LICENSE()
```

---

# 6. Module Initialization

The initialization function is called when the module is loaded.

```c
static int __init my_init(void)
{
    pr_info("Loaded\n");
    return 0;
}
```

Registered using:

```c
module_init(my_init);
```

Flow:

```text
insmod/modprobe
       |
       v
Kernel loads module
       |
       v
module_init()
       |
       v
my_init()
```

---

# 7. Module Exit

The exit function runs when the module is unloaded.

```c
static void __exit my_exit(void)
{
    pr_info("Unloaded\n");
}
```

Registered using:

```c
module_exit(my_exit);
```

Flow:

```text
rmmod
  |
  v
Module exit function
  |
  v
Resources released
  |
  v
Module removed
```

---

# 8. `__init`

You will commonly see:

```c
static int __init my_init(void)
```

`__init` marks initialization code.

After initialization is complete, the kernel can reclaim memory used exclusively by that initialization code.

Conceptually:

```text
Module load
    |
    v
Initialization code
    |
    v
Initialization complete
    |
    v
__init memory can be reclaimed
```

---

# 9. `__exit`

You may see:

```c
static void __exit my_exit(void)
```

`__exit` identifies cleanup code used when a module is unloaded.

If functionality is built directly into the kernel rather than as a loadable module, there is no normal module-unload path, so exit code can potentially be omitted.

---

# 10. Module Metadata

Typical metadata:

```c
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Author");
MODULE_DESCRIPTION("Example driver");
MODULE_VERSION("1.0");
```

This information can be inspected using tools such as:

```bash
modinfo my_driver
```

---

# 11. Building a Module

A typical external-module Makefile:

```make
obj-m += my_driver.o

all:
	make -C /lib/modules/$(shell uname -r)/build \
	     M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build \
	     M=$(PWD) clean
```

Build:

```bash
make
```

Output:

```text
my_driver.ko
```

---

# 12. Why Use the Kernel Build System?

Kernel modules must integrate with:

```text
Kernel configuration
Kernel headers
Compiler options
Architecture-specific settings
Kernel build infrastructure
Symbol information
```

Therefore, external modules normally use the kernel's Kbuild system.

---

# 13. `insmod`

`insmod` loads a module directly.

Example:

```bash
sudo insmod my_driver.ko
```

Conceptually:

```text
my_driver.ko
     |
     v
insmod
     |
     v
Kernel
     |
     v
module_init()
```

---

# 14. `rmmod`

`rmmod` removes a loaded module.

```bash
sudo rmmod my_driver
```

Flow:

```text
rmmod
  |
  v
module_exit()
  |
  v
Module removed
```

The module cannot normally be removed while it is still in use.

---

# 15. `modprobe`

`modprobe` is generally preferred for managing modules because it understands module dependencies.

Example:

```bash
sudo modprobe my_driver
```

Remove:

```bash
sudo modprobe -r my_driver
```

Conceptually:

```text
modprobe
   |
   +-- Find module
   +-- Resolve dependencies
   +-- Load required modules
   +-- Load requested module
```

---

# 16. `insmod` vs `modprobe`

| Feature                             | insmod         | modprobe  |
| ----------------------------------- | -------------- | --------- |
| Load module                         | Yes            | Yes       |
| Dependency handling                 | Minimal/direct | Yes       |
| Common system administration choice | Less common    | Preferred |
| Uses module metadata/configuration  | Limited        | Yes       |

Interview answer:

> `insmod` directly inserts a module, while `modprobe` understands dependencies and is generally preferred for normal module management.

---

# 17. `lsmod`

List currently loaded modules:

```bash
lsmod
```

Example:

```text
Module       Size    Used by
usb_storage  ...
xhci_hcd     ...
```

Conceptually:

```text
lsmod
   |
   v
Kernel module list
```

---

# 18. `/proc/modules`

Loaded-module information can also be viewed through:

```bash
cat /proc/modules
```

This exposes kernel-maintained information about loaded modules.

---

# 19. `modinfo`

Use:

```bash
modinfo my_driver
```

to inspect module metadata.

It can show information such as:

```text
filename
license
description
author
version
depends
alias
```

---

# 20. Module Dependencies

Suppose:

```text
Driver A
   |
   v
Driver B
```

A depends on B.

Therefore:

```text
Load A
   |
   v
Load B first
   |
   v
Load A
```

`modprobe` can resolve such dependencies.

---

# 21. Module Symbols

A module may need functions or variables exported by another kernel component.

Example:

```text
Module A
   |
   | uses symbol
   v
Module B
   |
   +-- EXPORT_SYMBOL()
```

The kernel resolves these symbols during module loading.

---

# 22. `EXPORT_SYMBOL`

A kernel component can export a symbol:

```c
EXPORT_SYMBOL(my_function);
```

Another module can use it:

```c
my_function();
```

Conceptually:

```text
Module A
   |
   | calls
   v
my_function()
   ^
   |
Module B exports it
```

---

# 23. `EXPORT_SYMBOL_GPL`

Linux also supports:

```c
EXPORT_SYMBOL_GPL(my_function);
```

This restricts the symbol to modules satisfying the kernel's GPL-compatible licensing rules.

Interview point:

```text
EXPORT_SYMBOL
        vs
EXPORT_SYMBOL_GPL
```

are not identical.

---

# 24. Symbol Resolution

When loading a module:

```text
Module
   |
   v
Undefined symbols
   |
   v
Kernel symbol table
   |
   v
Resolve dependencies
   |
   v
Relocate module
   |
   v
Ready to execute
```

If a required symbol cannot be resolved, module loading fails.

---

# 25. Module Parameters

Modules can accept parameters.

Example:

```c
static int debug = 0;

module_param(debug, int, 0644);
```

Load:

```bash
sudo modprobe my_driver debug=1
```

Conceptually:

```text
modprobe my_driver debug=1
              |
              v
        module parameter
```

---

# 26. Why Module Parameters?

Useful for:

```text
Debugging
Hardware configuration
Performance tuning
Optional behavior
Testing
```

Instead of recompiling:

```text
Change parameter
    ↓
Reload module
```

---

# 27. Module Reference Counting

The kernel must prevent a module from being unloaded while its code is still being used.

Conceptually:

```text
Module
  |
  +-- User 1
  +-- User 2
  +-- User 3

Reference count = 3
```

When users disappear:

```text
3 → 2 → 1 → 0
```

The module can then become removable.

---

# 28. Why Reference Counting Matters

Imagine:

```text
CPU 0
 |
 +-- executing driver_function()
```

while another CPU executes:

```text
rmmod driver
```

If the module memory were freed immediately:

```text
CPU 0
 |
 +-- executing code that no longer exists
```

This could cause a crash.

Reference/lifetime management prevents this class of problem.

---

# 29. Module Loading and Memory

A module is loaded into kernel memory.

Conceptually:

```text
Kernel Address Space
        |
        +-- Kernel text
        +-- Kernel data
        +-- Module text
        +-- Module data
```

The exact placement and memory-management details depend on architecture and kernel configuration.

---

# 30. Module Relocation

A module may contain addresses that need to be resolved when loaded.

The kernel loader performs relocation so that:

```text
Module code
    |
    v
Correct kernel/module addresses
```

This is similar in concept to dynamic linking in user space, but it occurs inside the kernel environment.

---

# 31. Module Loading Flow

Understand this flow:

```text
                 my_driver.ko
                      |
                      v
                  modprobe
                      |
                      v
             Read module metadata
                      |
                      v
             Resolve dependencies
                      |
                      v
               Allocate memory
                      |
                      v
                 Load module
                      |
                      v
              Resolve symbols
                      |
                      v
                 Relocation
                      |
                      v
              module_init()
                      |
                      v
              Driver active
```

---

# 32. Module Unloading Flow

```text
                  rmmod
                    |
                    v
             Check module usage
                    |
              +-----+-----+
              |           |
            Busy       Not busy
              |           |
              X           v
          Cannot      module_exit()
          unload          |
                          v
                    Release resources
                          |
                          v
                    Remove module
```

---

# 33. Module vs Driver

Important distinction:

> A kernel module is a mechanism for dynamically adding kernel code.

A device driver is code that controls/manages a device.

Therefore:

```text
Module ≠ Driver
```

A driver **can be built as a module**, but not every module is a device driver.

Examples of modules that may not be device drivers:

```text
Filesystem support
Security functionality
Other kernel extensions
```

---

# 34. Device Driver as a Module

Common structure:

```text
my_driver.ko
      |
      v
module_init()
      |
      v
register driver
      |
      v
Kernel/device subsystem
      |
      v
Hardware
```

For example:

```text
PCI driver
USB driver
Platform driver
I2C driver
SPI driver
Network driver
```

---

# 35. Module and Device Registration

A driver module often performs registration during initialization.

Conceptually:

```c
static int __init my_driver_init(void)
{
    return driver_register(...);
}
```

Then cleanup:

```c
static void __exit my_driver_exit(void)
{
    driver_unregister(...);
}
```

The exact API depends on the subsystem.

---

# 36. Module Initialization Failure

Suppose:

```c
static int __init my_init(void)
{
    allocate_resource();

    if (error)
        return -ENOMEM;

    register_driver();

    return 0;
}
```

If initialization fails:

```text
Partial initialization
       |
       v
Cleanup resources already acquired
       |
       v
Return error
```

A senior engineer must ensure failure paths correctly release previously allocated resources.

---

# 37. Error Handling

Typical pattern:

```text
Acquire A
   |
   v
Acquire B
   |
   v
Acquire C
   |
   X C fails
   |
   v
Release B
   |
   v
Release A
```

This is especially important in kernel code because resource leaks can affect the entire system.

---

# 38. `printk()` and `pr_*()`

Kernel modules cannot use ordinary user-space `printf()`.

Use kernel logging facilities:

```c
pr_info("Driver loaded\n");
pr_err("Hardware initialization failed\n");
pr_debug("Debug information\n");
```

Older code often uses:

```c
printk(KERN_INFO "Driver loaded\n");
```

The `pr_*()` family is generally clearer for many cases.

---

# 39. Viewing Kernel Logs

Use:

```bash
dmesg
```

or:

```bash
journalctl -k
```

Example:

```bash
dmesg | tail
```

Useful when debugging:

```text
Module loading
Driver probing
Hardware failures
Interrupt problems
Kernel warnings
```

---

# 40. Module Tainting

The kernel tracks certain conditions using a taint state.

For example, loading certain proprietary/out-of-tree modules can affect the kernel's taint state.

Check:

```bash
cat /proc/sys/kernel/tainted
```

Why?

Because when diagnosing a kernel failure, maintainers need to know whether unsupported or external code is involved.

---

# 41. Out-of-Tree Module

An out-of-tree module is built separately from the kernel's main source tree.

Examples:

```text
Vendor driver
Custom driver
Development driver
Third-party kernel module
```

It is commonly built using:

```text
Kbuild
```

against the target kernel's build environment.

---

# 42. In-Tree Module

An in-tree module is maintained within the Linux kernel source tree.

Conceptually:

```text
Linux source tree
   |
   +-- driver
   +-- subsystem
   +-- module
```

Advantages include better integration with:

```text
Kernel APIs
Build system
Testing
Maintenance
Kernel releases
```

---

# 43. Kernel Version Compatibility

Kernel modules are tightly coupled to kernel internals.

A module built for one kernel version may not work with another.

Linux can use:

```text
vermagic
```

and symbol/version information to detect compatibility issues.

Check:

```bash
modinfo my_driver
```

---

# 44. Why Kernel APIs Matter

Kernel APIs can change.

Example:

```text
Kernel version A
    |
    +-- API X

Kernel version B
    |
    +-- API changed
```

An out-of-tree module may require source changes to compile against a newer kernel.

This is a major maintenance issue for vendor drivers.

---

# 45. Module Signing

Linux can enforce module signing.

Conceptually:

```text
module.ko
    |
    v
Signature verification
    |
    +---- valid ----> Load
    |
    +---- invalid --> Reject
```

This is important for kernel security.

---

# 46. Secure Boot

On systems using Secure Boot, kernel module loading can be subject to signature/trust requirements.

Conceptually:

```text
UEFI Secure Boot
       |
       v
Trusted boot chain
       |
       v
Kernel
       |
       v
Trusted module
```

A module that is not trusted/signed appropriately may fail to load depending on system configuration.

---

# 47. Module Autoloading

Linux can automatically load modules when required.

Example:

```text
Hardware detected
       |
       v
Kernel generates device information/alias
       |
       v
Userspace/module loader
       |
       v
modprobe
       |
       v
Required module loaded
```

This is one reason `modprobe` and module aliases are important.

---

# 48. Module Aliases

A module can advertise hardware identifiers it supports.

Conceptually:

```text
Hardware ID
     |
     v
Alias
     |
     v
Matching module
```

This allows automatic driver loading.

For example, USB/PCI device IDs can map hardware to a driver.

---

# 49. `modinfo` and Aliases

You can inspect aliases with:

```bash
modinfo <module>
```

Look for:

```text
alias:
```

This is particularly useful when debugging:

> Why did Linux not automatically load my driver?

---

# 50. Module Dependencies and `modprobe`

Suppose:

```text
Driver A
   |
   +-- requires subsystem module B
```

Then:

```bash
modprobe A
```

can load:

```text
B
↓
A
```

Whereas:

```bash
insmod A.ko
```

does not provide the same dependency-resolution behavior.

---

# 51. Kernel Module Security

Loading arbitrary kernel code is extremely powerful.

A malicious module can potentially:

```text
Access kernel memory
Modify kernel behavior
Intercept operations
Compromise the system
```

Therefore:

```text
Module signing
Secure Boot
Restricted module loading
Kernel lockdown mechanisms
```

can be important security controls.

---

# 52. Module Parameters and Security

Be careful with parameters that affect:

```text
Debugging
Memory access
Hardware behavior
Security settings
```

Module parameters are kernel inputs and should be validated appropriately.

---

# 53. Concurrency in Modules

Kernel modules execute concurrently.

A driver may have:

```text
Process context
IRQ context
Workqueue
Kernel thread
Multiple CPUs
```

Therefore module code must use synchronization correctly.

Example:

```text
Driver
 |
 +-- read()
 |
 +-- write()
 |
 +-- IRQ handler
 |
 +-- workqueue
 |
 +-- timer
```

All may interact with shared driver state.

---

# 54. Module + Synchronization

A typical driver might use:

```text
Mutex
    ↓
Protect process-context operations

Spinlock
    ↓
Protect IRQ/shared fast state

Completion
    ↓
Wait for hardware event

Atomic/refcount
    ↓
Simple counters/lifetime
```

The correct primitive depends on context.

---

# 55. Module + Interrupt

A driver module may register an interrupt handler:

```text
module_init()
     |
     v
Register driver
     |
     v
Hardware interrupt occurs
     |
     v
IRQ handler
```

The IRQ handler must follow interrupt-context restrictions.

It cannot simply perform arbitrary operations that may sleep.

---

# 56. Module + Workqueue

Typical design:

```text
Hardware
   |
   v
IRQ
   |
   +-- acknowledge interrupt
   +-- capture status
   +-- schedule work
             |
             v
         Workqueue
             |
             +-- longer processing
             +-- operations that may sleep
```

This pattern appears frequently in real Linux drivers.

---

# 57. Module Cleanup

A good module exit function must undo initialization in reverse order.

If initialization does:

```text
A
B
C
D
```

cleanup should generally be:

```text
D
C
B
A
```

Example:

```text
register driver
allocate memory
request IRQ
create device interface
```

cleanup:

```text
remove interface
free IRQ
free memory
unregister driver
```

The exact order depends on resource dependencies.

---

# 58. Common Module Bugs

### Bug 1: Resource leak

```text
allocate()
    |
    X initialization fails
    |
    +-- resource not freed
```

---

### Bug 2: Use-after-free

```text
Module unload
     |
     v
Memory freed
     |
     v
Another execution context uses it
```

---

### Bug 3: Incorrect synchronization

```text
Process
   |
   +-- modifies state

IRQ
   |
   +-- modifies same state

No proper locking
```

---

### Bug 4: Incorrect cleanup

```text
Initialization order
A → B → C

Cleanup
A → B → C
```

This can violate dependencies.

Prefer appropriate reverse dependency order:

```text
C → B → A
```

---

# 59. Module Debugging Workflow

When a module fails:

```text
1. Check kernel version
2. Check module exists
3. Run modinfo
4. Check dependencies
5. Try modprobe
6. Check dmesg
7. Check module parameters
8. Check symbols
9. Check signature/taint issues
10. Check hardware/device matching
```

Useful commands:

```bash
uname -r
modinfo my_driver
lsmod
dmesg
journalctl -k
modprobe my_driver
```

---

# 60. Senior Interview Scenario

### Question:

A driver module builds successfully but fails to load.

What do you check?

Answer:

```text
1. Kernel version
2. vermagic compatibility
3. Required symbols
4. Module dependencies
5. Module signature
6. Kernel configuration
7. Architecture
8. Device/hardware matching
9. Module parameters
10. Kernel log
```

Start with:

```bash
dmesg
```

because the kernel usually provides the actual loading error.

---

# 61. Senior Interview Scenario

### Question:

Why does `insmod` fail while `modprobe` succeeds?

Possible reason:

```text
insmod
    |
    +-- directly inserts specified .ko

modprobe
    |
    +-- understands dependencies
    +-- resolves module configuration
    +-- can load required modules
```

Therefore a dependency may be missing when using `insmod`.

---

# 62. Senior Interview Scenario

### Question:

Can every kernel module be unloaded?

No.

A module may not be removable when:

```text
It is in use
References remain
Dependent modules exist
Unload is disabled
Module has no appropriate unload path
Kernel configuration/design prevents unloading
```

---

# 63. Senior Interview Scenario

### Question:

Why can unloading a driver be dangerous?

Because other execution contexts may still reference:

```text
Driver code
Driver data
Interrupt handlers
Workqueues
Timers
File operations
Device objects
```

Cleanup must ensure all users and asynchronous activity have stopped before freeing resources.

---

# 64. Module Lifetime

Think about module lifetime as:

```text
                LOAD
                  |
                  v
              INIT
                  |
                  v
             REGISTER
                  |
                  v
              ACTIVE
                  |
        +---------+---------+
        |                   |
     IN USE              UNUSED
        |                   |
        |                   v
        +------------>  EXIT
                           |
                           v
                        CLEANUP
                           |
                           v
                        UNLOAD
```

---

# 65. Module vs User-Space Shared Library

This is a common interview comparison.

### User-space `.so`

```text
Application
    |
    v
Dynamic linker
    |
    v
User-space library
```

### Kernel `.ko`

```text
Kernel
    |
    v
Kernel module loader
    |
    v
Kernel module
```

A `.ko` executes with kernel privileges.

Therefore a kernel module failure can crash or corrupt the entire system.

---

# 66. Why Kernel Module Bugs Are More Dangerous

User-space crash:

```text
Application
    |
    X
```

Usually affects that process.

Kernel module crash:

```text
Kernel module
      |
      X
      |
      v
Kernel
      |
      v
Entire system affected
```

This is why kernel development requires much stricter resource, lifetime, synchronization, and error handling.

---

# 67. Module Architecture

A typical driver module can be viewed as:

```text
                 module.ko
                    |
          +---------+---------+
          |                   |
       Init/Cleanup        Driver Logic
          |                   |
          v                   v
    Registration          Device Operations
                              |
                +-------------+-------------+
                |             |             |
              read()       write()        ioctl()
                |
                v
             Hardware
                |
                v
               IRQ
                |
                v
          Deferred Work
```

---

# 68. What You Must Know for Senior Interviews

### Must Know

```text
★★★★★ What is a kernel module?
★★★★★ .ko
★★★★★ module_init()
★★★★★ module_exit()
★★★★★ __init / __exit
★★★★★ insmod
★★★★★ modprobe
★★★★★ rmmod
★★★★★ lsmod
★★★★★ modinfo
★★★★★ Module dependencies
★★★★★ EXPORT_SYMBOL
★★★★★ Module parameters
★★★★★ Reference/lifetime management
★★★★★ Module loading flow
★★★★★ Module unloading flow
★★★★★ Error handling
★★★★★ Driver as a module
★★★★★ Module signing
★★★★★ Out-of-tree modules
```

### Good to Know

```text
★★★★☆ Module aliases
★★★★☆ vermagic
★★★★☆ Symbol resolution
★★★★☆ Relocation
★★★★☆ Kernel tainting
★★★★☆ Autoloading
★★★★☆ Secure Boot
★★★★☆ Kbuild
```

---

# 69. Most Important Commands

```bash
# Kernel version
uname -r

# List loaded modules
lsmod

# Module information
modinfo my_driver

# Load module
sudo modprobe my_driver

# Remove module
sudo modprobe -r my_driver

# Direct insertion
sudo insmod my_driver.ko

# Direct removal
sudo rmmod my_driver

# Kernel logs
dmesg

# Kernel logs through systemd
journalctl -k
```

---

# 70. Final Mental Model

Memorize this:

```text
                 KERNEL MODULE
                      |
                      v
                  .ko file
                      |
                      v
                  modprobe
                      |
          +-----------+-----------+
          |                       |
     Dependencies             Symbols
          |                       |
          +-----------+-----------+
                      |
                      v
                  Load Module
                      |
                      v
                module_init()
                      |
                      v
               Register Driver
                      |
                      v
                    ACTIVE
                      |
          +-----------+-----------+
          |           |           |
        Process       IRQ      Workqueue
          |           |           |
          +-----------+-----------+
                      |
                      v
                Shared State
                      |
                      v
                Synchronization
                      |
                      v
                   Cleanup
                      |
                      v
                module_exit()
                      |
                      v
                   UNLOAD
```

---

# Chapter 14 Summary

The most important thing to understand is:

> **A kernel module is dynamically loadable kernel code, and a driver is one common type of code that can be packaged as a module.**

The complete lifecycle is:

```text
Source Code
    ↓
Kbuild
    ↓
module.ko
    ↓
modprobe
    ↓
Dependency Resolution
    ↓
Symbol Resolution
    ↓
Memory/Relocation
    ↓
module_init()
    ↓
Driver Registration
    ↓
Device Operation
    ↓
Synchronization / IRQ / Workqueue
    ↓
Cleanup
    ↓
module_exit()
    ↓
Module Unloaded
```

For **Qualcomm, AMD, NVIDIA, Intel and senior Linux/embedded roles**, don't stop at knowing `insmod`, `rmmod`, and `modprobe`. You should be able to explain **module lifetime, symbol resolution, dependencies, driver registration, concurrency, interrupt/workqueue interaction, cleanup ordering, and why unloading a module safely is difficult**.
