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
