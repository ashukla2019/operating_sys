# Application → System Call → Physical Device

## General System Call Flow

```text
APPLICATION
    |
    | calls
    v
LIBRARY FUNCTION
(e.g., read(), open(), write())
    |
    | calls
    v
SYSTEM-CALL WRAPPER
    |
    | prepares system-call number + arguments
    v
CPU REGISTERS
    |
    | wrapper executes
    v
SYSTEM-CALL INSTRUCTION
(e.g., syscall)
    |
    | causes
    v
CPU SWITCHES
USER MODE -> KERNEL MODE
    |
    | enters
    v
SYSTEM-CALL HANDLER / SYSTEM-CALL INTERFACE
    |
    | uses system-call number to look up
    v
SYSTEM-CALL TABLE
    |
    | selects
    v
CORRESPONDING KERNEL SYSTEM-CALL ROUTINE
(e.g., sys_read(), sys_open(), sys_write())
    |
    | may call
    v
KERNEL SUBSYSTEM
    |
    | if hardware access is required
    v
DEVICE DRIVER
    |
    | communicates with
    v
DEVICE CONTROLLER
    |
    | controls
    v
PHYSICAL DEVICE
```
---------------------------------------------------------------------------
## Important Notes
The application calls the library function.
The library function / wrapper prepares the system call.
The wrapper places the system-call number and arguments in CPU registers.
The wrapper executes a system-call instruction such as syscall.
The CPU hardware switches from User Mode to Kernel Mode.
The system-call handler receives the request.
The handler uses the system-call number to look up the system-call table.
The appropriate kernel system-call routine is selected.
The kernel routine may call a kernel subsystem, such as the file system.
If hardware access is required, the kernel subsystem may call a device driver.
The device driver communicates with the device controller.
The device controller controls the physical device.

-------------------------------------------------------------
**Why monolithic is faster than micorservice but microservice is more reiable?**
Monolithic is faster: Components usually communicate through direct function calls within the same process, avoiding network/serialization overhead.
Microservices can be more reliable: Services are isolated. If one service fails, other services can often continue working instead of the entire application going down.
----------------------------------------------------------------------
**Loadable Kernel Modules (LKM)**
Linux can load/unload drivers without rebooting.
Example: USB Driver → Load Module → Kernel uses Driver
Commands: lsmod, insmod, rmmod, modprobe
Advantages: No reboot, smaller kernel, easier driver updates.
Flow: Driver.ko → insmod → Kernel → Driver Initialized → Device Ready
