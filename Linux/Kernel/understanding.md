**System call working flow:**
APPLICATION
   │
   │ calls
   ↓
LIBRARY FUNCTION
(e.g., read(), open(), write())
   │
   │ calls
   ↓
SYSTEM-CALL WRAPPER
   │
   │ prepares system-call number + arguments
   ↓
CPU REGISTERS
   │
   │ wrapper executes
   ↓
SYSTEM-CALL INSTRUCTION
(e.g., syscall)
   │
   │ causes
   ↓
CPU SWITCHES
USER MODE → KERNEL MODE
   │
   │ enters
   ↓
SYSTEM-CALL HANDLER / SYSTEM-CALL INTERFACE
   │
   │ uses system-call number to look up
   ↓
SYSTEM-CALL TABLE
   │
   │ selects/calls
   ↓
CORRESPONDING KERNEL SYSTEM-CALL ROUTINE
(e.g., sys_read(), sys_open(), sys_write())
   │
   │ may call
   ↓
KERNEL SUBSYSTEM
   │
   │ if hardware access is required, calls
   ↓
DEVICE DRIVER
   │
   │ communicates with
   ↓
DEVICE CONTROLLER
   │
   │ controls
   ↓
PHYSICAL DEVICE
