# Chapter 8 – Linux Interrupts

---

# 1. What Is an Interrupt?

An interrupt is a mechanism by which hardware or software requests CPU attention.

Without interrupts, the CPU would need to continuously check devices:

```text
CPU
 |
 +-- Check NIC
 +-- Check Disk
 +-- Check UART
 +-- Check Timer
 +-- Check USB
 +-- Repeat...
```

This is inefficient.

With interrupts:

```text
CPU
 |
 | executes normal work
 |
 v
Hardware event
 |
 | IRQ
 v
CPU
 |
 v
Interrupt Handler
```

The CPU can perform other work until the device actually needs attention.

---

# 2. Why Are Interrupts Needed?

Consider a network card.

A packet arrives:

```text
NIC
 |
 | packet arrives
 v
?
```

Without interrupts:

```text
CPU
 |
 +-- Is packet available?
 +-- Is packet available?
 +-- Is packet available?
 +-- Is packet available?
```

This is polling.

With interrupts:

```text
CPU
 |
 | doing other work
 |
 v
NIC
 |
 | packet arrives
 |
 | IRQ
 v
CPU
 |
 v
Network Driver
```

The CPU is notified only when necessary.

---

# 3. Basic Interrupt Flow

The fundamental flow is:

```text
Hardware
    |
    | Interrupt Request
    v
Interrupt Controller
    |
    v
CPU
    |
    v
Kernel Interrupt Entry
    |
    v
Interrupt Handler
    |
    v
Driver
```

The exact hardware details vary by architecture.

---

# 4. IRQ

IRQ means:

```text
Interrupt Request
```

A device can generate an interrupt request.

Examples:

```text
NIC       → packet received
NVMe      → I/O completed
UART      → data received
Timer     → timer expired
GPU       → command completed
USB       → transfer completed
```

---

# 5. Interrupt Controller

The CPU normally does not directly manage every device interrupt.

An interrupt controller receives interrupt requests and routes them appropriately.

Conceptually:

```text
              Devices
          /      |      \
         v       v       v
       NIC     NVMe    UART
         \       |       /
          \      |      /
           v     v     v
        Interrupt Controller
                 |
                 v
                CPU
```

On modern systems there can be multiple interrupt-controller layers.

---

# 6. Interrupt Number

Linux identifies interrupts using IRQ numbers.

You can inspect interrupts using:

```bash
cat /proc/interrupts
```

Example:

```text
           CPU0       CPU1
  40:       100         50   NIC
  41:        20         30   NVMe
```

The exact output depends on the machine.

---

# 7. `/proc/interrupts`

This is an extremely useful debugging interface.

```bash
cat /proc/interrupts
```

It can show:

```text
IRQ number
Interrupt count
Per-CPU interrupt distribution
Interrupt controller information
Device/driver association
```

For example:

```text
            CPU0    CPU1    CPU2    CPU3
IRQ 40       10      20      15      18
```

This can help identify:

```text
Interrupt imbalance
Interrupt storms
CPU affinity problems
Unexpected interrupt activity
```

---

# 8. Interrupt Handler

A driver registers an interrupt handler.

Conceptually:

```c
request_irq(irq, handler, ...);
```

The handler is invoked when the corresponding interrupt occurs.

Conceptually:

```text
Device
   |
   v
IRQ
   |
   v
handler()
```

---

# 9. Interrupt Handler Responsibilities

An interrupt handler should normally perform only urgent work.

Typical tasks:

```text
1. Determine interrupt source
2. Acknowledge/clear interrupt
3. Read minimal device status
4. Capture necessary information
5. Schedule deferred processing
6. Return quickly
```

Avoid doing large amounts of work directly in hard interrupt context.

---

# 10. Why Must Interrupt Handlers Be Fast?

Suppose an interrupt handler takes a long time:

```text
IRQ
 |
 v
Long handler
 |
 +--------------------+
 | CPU unavailable    |
 +--------------------+
```

Other work can be delayed.

High interrupt latency can affect:

```text
Networking
Audio
Storage
Real-time workloads
System responsiveness
```

Therefore:

```text
Interrupt Handler
       |
       v
Do minimal work
       |
       v
Defer expensive work
```

---

# 11. Hard IRQ Context

The immediate interrupt handler runs in interrupt context.

Conceptually:

```text
Normal execution
      |
      v
Hardware IRQ
      |
      v
Hard IRQ context
      |
      v
Return
      |
      v
Normal execution
```

Important rule:

> Code executing in hard interrupt context must not sleep.

---

# 12. Why Can't IRQ Handlers Sleep?

Sleeping means:

```text
Current execution
       |
       v
Wait for something
       |
       v
Scheduler chooses another task
```

But an interrupt handler is not running as a normal schedulable process.

Therefore operations that may sleep are not allowed in hard IRQ context.

For example, generally avoid:

```text
mutex_lock()
kmalloc(..., GFP_KERNEL)
blocking I/O
wait_event()
```

in hard IRQ context.

---

# 13. Interrupt Context vs Process Context

This distinction is critical.

### Process Context

```text
System call
   |
   v
Driver
```

The driver is executing on behalf of a process.

It can generally:

```text
Sleep
Block
Use mutexes
Perform blocking operations
```

### Interrupt Context

```text
Hardware
   |
   v
IRQ Handler
```

The handler:

```text
Cannot sleep
Must execute quickly
Uses atomic/IRQ-safe synchronization
```

---

# 14. Top Half

Historically, interrupt processing was divided into:

```text
Top Half
Bottom Half
```

The top half executes immediately when the interrupt occurs.

Typical work:

```text
Top Half
   |
   +-- Acknowledge interrupt
   +-- Read status
   +-- Save minimal information
   +-- Schedule deferred work
```

Then it returns quickly.

---

# 15. Bottom Half

The bottom half handles work that does not need to happen immediately.

Conceptually:

```text
Hardware
   |
   v
Top Half
   |
   v
Bottom Half
   |
   v
Longer Processing
```

Linux provides several mechanisms for deferred work.

---

# 16. Deferred Interrupt Processing

Important mechanisms include:

```text
Softirqs
Tasklets
Workqueues
Threaded IRQs
```

You should understand the differences rather than memorizing old APIs.

---

# 17. Softirq

Softirqs are a mechanism for deferred kernel work.

They run in an atomic context and therefore cannot sleep.

Conceptually:

```text
Hard IRQ
   |
   v
Softirq
   |
   v
Deferred processing
```

Softirqs are used by important kernel subsystems, including networking.

---

# 18. Tasklets

Tasklets were historically used for deferred interrupt processing.

Conceptually:

```text
Hard IRQ
   |
   v
Tasklet
   |
   v
Deferred work
```

Important interview point:

> Tasklets cannot sleep.

For modern driver development, workqueues and threaded IRQs are often more relevant.

---

# 19. Workqueue

A workqueue executes deferred work in process context.

Conceptually:

```text
IRQ
 |
 v
Schedule Work
 |
 v
Workqueue
 |
 v
Worker Thread
 |
 v
Driver Processing
```

Because it runs in process context, the work can generally sleep when appropriate.

---

# 20. Threaded IRQ

Linux supports threaded interrupt handlers.

Conceptually:

```text
Hardware IRQ
     |
     v
Primary IRQ Handler
     |
     v
IRQ Thread
     |
     v
Longer Processing
```

This is useful when interrupt processing requires operations that can sleep.

---

# 21. Comparing Deferred Mechanisms

| Mechanism    |           Can Sleep? | Typical Use                           |
| ------------ | -------------------: | ------------------------------------- |
| Hard IRQ     |                   No | Immediate interrupt handling          |
| Softirq      |                   No | High-performance deferred kernel work |
| Tasklet      |                   No | Legacy/simple deferred work           |
| Workqueue    |                  Yes | Deferred process-context work         |
| Threaded IRQ | Yes in threaded part | Device interrupt processing           |

The exact kernel execution context and rules matter more than memorizing the table.

---

# 22. Interrupt Handler Example

Conceptual driver:

```c
irqreturn_t my_irq_handler(int irq, void *data)
{
    struct device_data *dev = data;

    /*
     * Read device status
     */
    status = readl(dev->base + STATUS);

    /*
     * Acknowledge interrupt
     */
    writel(status, dev->base + IRQ_ACK);

    /*
     * Defer expensive work
     */
    schedule_work(&dev->work);

    return IRQ_HANDLED;
}
```

The important architecture is:

```text
IRQ
 |
 +-- Read status
 +-- Acknowledge
 +-- Schedule work
 |
 v
Return quickly
```

---

# 23. IRQ_RETURN Values

An interrupt handler commonly returns:

```text
IRQ_HANDLED
```

when it handled the interrupt.

It can return:

```text
IRQ_NONE
```

when the interrupt was not from that device.

Conceptually:

```text
IRQ
 |
 v
Handler
 |
 +-- Device caused it?
 |       |
 |       +-- YES → IRQ_HANDLED
 |       |
 |       +-- NO  → IRQ_NONE
```

This is particularly relevant for shared interrupts.

---

# 24. Shared Interrupts

Multiple devices can sometimes share an interrupt line.

Conceptually:

```text
Device A ----+
             |
Device B ----+---- IRQ ----> CPU
             |
Device C ----+
```

The handlers need to determine whether their device generated the interrupt.

```text
Handler A → check device A
Handler B → check device B
Handler C → check device C
```

If a handler did not handle the interrupt:

```text
IRQ_NONE
```

can be returned.

---

# 25. Interrupt Storm

An interrupt storm occurs when a device generates interrupts excessively.

Conceptually:

```text
Device
 |
 +-- IRQ
 +-- IRQ
 +-- IRQ
 +-- IRQ
 +-- IRQ
 +-- IRQ
 ...
```

The CPU spends too much time handling interrupts.

Symptoms can include:

```text
High CPU usage
Poor application performance
High interrupt latency
System instability
```

---

# 26. Causes of Interrupt Storms

Possible causes:

```text
Interrupt not acknowledged
Interrupt status not cleared
Hardware malfunction
Driver bug
Incorrect interrupt configuration
Device repeatedly reporting the same event
```

Debug with:

```bash
cat /proc/interrupts
```

and driver logs/tracing.

---

# 27. Interrupt Affinity

On multicore systems, interrupts can be routed to particular CPUs.

Conceptually:

```text
NIC IRQ
   |
   +---- CPU 2
```

or:

```text
NIC
 |
 +-- RX queue 0 → CPU 0
 +-- RX queue 1 → CPU 1
 +-- RX queue 2 → CPU 2
 +-- RX queue 3 → CPU 3
```

This is important for high-performance networking and storage.

---

# 28. `/proc/irq`

Linux exposes interrupt configuration through:

```text
/proc/irq/
```

For example:

```text
/proc/irq/<IRQ>/
```

Information can include:

```text
Affinity
Interrupt controller information
Statistics
```

Depending on kernel configuration.

---

# 29. SMP and Interrupts

On a multicore system:

```text
              Interrupt Controller
               /       |       \
              v        v        v
            CPU0     CPU1     CPU2
```

The kernel must coordinate interrupt processing across CPUs.

This introduces concurrency issues.

---

# 30. Interrupts and Locking

Suppose a driver shares data between:

```text
Process context
```

and:

```text
Interrupt context
```

A normal mutex may not be appropriate because the interrupt handler cannot sleep.

The driver may need an IRQ-safe locking strategy.

For example:

```c
spin_lock_irqsave(&lock, flags);
...
spin_unlock_irqrestore(&lock, flags);
```

This is a common pattern when the same lock can be accessed from interrupt and process context.

---

# 31. Why `spin_lock_irqsave()`?

Consider:

```text
CPU
 |
 +-- Process context holds lock
 |
 +-- IRQ arrives
       |
       +-- IRQ handler tries same lock
```

If the interrupt handler spins waiting for a lock held by the interrupted code, the CPU can deadlock.

Disabling local interrupts while holding the lock can prevent this specific re-entry scenario.

Conceptually:

```text
Process Context
      |
      v
Disable local IRQs
      |
      v
Acquire spinlock
      |
      v
Critical Section
      |
      v
Release lock
      |
      v
Restore IRQ state
```

The exact locking strategy must match where the lock is used.

---

# 32. Spinlock in Interrupt Context

A spinlock is appropriate when:

```text
Critical section is short
Code cannot sleep
```

Example:

```text
IRQ Handler
    |
    v
spin_lock()
    |
    v
update shared state
    |
    v
spin_unlock()
```

Do not perform long operations while holding a spinlock.

---

# 33. Interrupt Latency

Interrupt latency is the time between:

```text
Interrupt occurs
```

and:

```text
Interrupt handler starts processing
```

Conceptually:

```text
IRQ occurs
   |
   | latency
   v
Handler begins
```

Low latency is important for:

```text
Real-time systems
Audio
Control systems
High-performance devices
```

---

# 34. Interrupt Processing Time

Two separate concepts:

### Interrupt latency

```text
IRQ → handler starts
```

### Interrupt handling time

```text
Handler starts → handler completes
```

A system can have:

```text
Low latency
but
Long handler execution
```

or the reverse.

---

# 35. Interrupt Coalescing

High-speed devices can reduce interrupt frequency by combining multiple events.

Without coalescing:

```text
Packet → IRQ
Packet → IRQ
Packet → IRQ
Packet → IRQ
```

With coalescing:

```text
Packet
Packet
Packet
Packet
   |
   v
One IRQ
```

Benefits:

```text
Lower interrupt overhead
Higher throughput
```

Trade-off:

```text
Potentially higher latency
```

This is widely used in NICs and other high-throughput devices.

---

# 36. MSI

PCI/PCIe devices can use:

```text
Message Signaled Interrupts
```

Instead of relying on a traditional physical interrupt line, the device generates an interrupt through a memory transaction mechanism.

Conceptually:

```text
PCIe Device
     |
     | MSI
     v
Interrupt System
     |
     v
CPU
```

---

# 37. MSI-X

MSI-X supports multiple interrupt vectors.

This is especially useful for devices with multiple queues.

Example:

```text
NIC
 |
 +-- RX Queue 0 → IRQ 0
 +-- RX Queue 1 → IRQ 1
 +-- RX Queue 2 → IRQ 2
 +-- RX Queue 3 → IRQ 3
```

These can be distributed across CPUs.

---

# 38. NIC Interrupt Flow

A modern network receive path can look like:

```text
Network Packet
      |
      v
      NIC
      |
      | DMA
      v
   RX Ring
      |
      v
   Interrupt
      |
      v
Network Driver
      |
      v
Deferred/NAPI Processing
      |
      v
Network Stack
      |
      v
Socket
      |
      v
Application
```

This connects interrupts with:

```text
DMA
Networking
Scheduling
```

---

# 39. NAPI

Linux networking uses NAPI to reduce interrupt overhead under high packet rates.

Conceptually:

```text
Low traffic
    |
    v
Interrupt
    |
    v
Process packets
```

Under high traffic:

```text
Interrupt
    |
    v
Disable/reduce further RX interrupts
    |
    v
Polling
    |
    v
Process batch of packets
    |
    v
Re-enable interrupts
```

The goal is to combine interrupt-driven notification with polling for efficient packet processing.

---

# 40. Why NAPI?

Suppose traffic is extremely high:

```text
Packet
 IRQ
Packet
 IRQ
Packet
 IRQ
Packet
 IRQ
...
```

Interrupt overhead becomes excessive.

NAPI allows:

```text
IRQ
 |
 v
Poll many packets
 |
 v
Batch processing
```

This improves throughput and reduces interrupt overhead.

---

# 41. Storage Interrupt Example

Consider NVMe.

```text
Application
    |
    v
Filesystem
    |
    v
Block Layer
    |
    v
NVMe Driver
    |
    v
NVMe Controller
    |
    | DMA
    v
Memory
    |
    v
Completion
    |
    v
MSI-X Interrupt
    |
    v
Driver
    |
    v
Complete I/O
```

This is a very important senior Linux/storage flow.

---

# 42. Interrupt + DMA Relationship

A common hardware pattern is:

```text
Driver
   |
   | Configure DMA
   v
Device
   |
   | DMA transfer
   v
RAM
   |
   | Transfer complete
   v
IRQ
   |
   v
Driver
```

The CPU is not required to copy every byte.

---

# 43. Interrupt + Wait Queue

Suppose an application waits for device data.

```text
Application
    |
    v
read()
    |
    v
Wait Queue
    |
    v
Sleep
```

Device receives data:

```text
Hardware
   |
   v
IRQ
   |
   v
Driver
   |
   v
Wake Up
   |
   v
Application
```

The application becomes runnable again.

---

# 44. Interrupt + Completion

Another common pattern:

```text
Process Context
      |
      v
Start hardware operation
      |
      v
wait_for_completion()
      |
      v
Sleep
```

Hardware finishes:

```text
Hardware
   |
   v
IRQ
   |
   v
Driver
   |
   v
complete()
   |
   v
Wake process
```

This is a clean synchronization model for device operations.

---

# 45. Interrupt Safety Rules

In hard interrupt context:

```text
DO:
    Keep handler short
    Use atomic/IRQ-safe synchronization
    Acknowledge interrupt
    Schedule deferred work
    Update protected state

DON'T:
    Sleep
    Block
    Take a mutex that may sleep
    Perform long operations
    Perform unnecessary allocations
```

---

# 46. Common Interrupt Bugs

### Bug 1 – Interrupt not cleared

```text
Device
 |
 v
IRQ
 |
 v
Handler
 |
 X
interrupt remains active
 |
 v
IRQ again
 |
 v
IRQ again
```

Result:

```text
Interrupt storm
```

---

### Bug 2 – Sleeping in IRQ

```text
IRQ Handler
    |
    v
Blocking operation
    |
    X
Invalid context
```

Can produce warnings or crashes.

---

### Bug 3 – Race with shared state

```text
CPU 0                IRQ
  |                    |
  | modify state       |
  |                    | read state
  |                    |
  +--------------------+
```

Without proper synchronization, the interrupt may observe inconsistent data.

---

### Bug 4 – Excessive handler work

```text
IRQ
 |
 v
Huge processing
 |
 v
High latency
```

Move expensive work to an appropriate deferred context.

---

# 47. Debugging Interrupt Problems

First check:

```bash
cat /proc/interrupts
```

Look for:

```text
Unexpectedly high interrupt counts
One CPU receiving all interrupts
Interrupt count not increasing
Interrupt count increasing too rapidly
```

Then inspect:

```text
dmesg
/sys
/proc/irq
ftrace
tracepoints
perf
```

---

# 48. Interrupt Debugging Example

Suppose CPU usage is 100%.

Check:

```bash
cat /proc/interrupts
```

You find:

```text
IRQ 45
CPU0: 50000000
CPU1: 10
```

Possible suspicion:

```text
Interrupt storm
```

Next investigate:

```text
Which device owns IRQ 45?
Is the interrupt being acknowledged?
Is the device continuously generating events?
Is IRQ affinity correct?
Is the driver stuck?
```

---

# 49. Senior Interview Scenario

### Question:

A device driver causes CPU usage to reach 100%. How would you debug it?

Answer structure:

```text
1. Check /proc/interrupts
2. Identify rapidly increasing IRQ
3. Identify device/driver
4. Check whether interrupt is being acknowledged
5. Check driver logs
6. Check IRQ affinity
7. Check for interrupt storm
8. Inspect handler/deferred work
9. Trace interrupt activity if necessary
10. Check device/hardware state
```

This is much stronger than simply saying:

> "I would check the CPU."

---

# 50. Interrupt Mental Model

Memorize:

```text
                    HARDWARE
                        |
                        | IRQ
                        v
                INTERRUPT CONTROLLER
                        |
                        v
                       CPU
                        |
                        v
                 HARD IRQ HANDLER
                        |
             +----------+----------+
             |                     |
        Immediate work       Deferred work
                                   |
                   +---------------+---------------+
                   |               |               |
                Softirq        Workqueue      IRQ Thread
                   |               |               |
                   +---------------+---------------+
                                   |
                                   v
                              DRIVER STATE
                                   |
                                   v
                              USER SPACE
```

---

# 51. Important Interview Questions

## Q1. What is an interrupt?

A mechanism that allows hardware/software to request CPU attention asynchronously.

---

## Q2. Why use interrupts instead of polling?

Interrupts allow the CPU to perform useful work until an event occurs, reducing unnecessary CPU usage.

---

## Q3. Can an interrupt handler sleep?

No, hard interrupt context cannot sleep.

---

## Q4. What is a bottom half?

A mechanism for deferring interrupt-related processing so the hard interrupt handler can return quickly.

---

## Q5. Softirq vs workqueue?

```text
Softirq    → atomic context, cannot sleep
Workqueue  → process context, can generally sleep
```

---

## Q6. What is an interrupt storm?

A situation where interrupts occur excessively, consuming significant CPU time.

---

## Q7. How do you detect an interrupt storm?

Start with:

```bash
cat /proc/interrupts
```

and identify IRQs whose counters are increasing abnormally fast.

---

## Q8. What is interrupt affinity?

The CPU or set of CPUs to which an interrupt can be routed.

---

## Q9. What is MSI-X?

A PCI/PCIe interrupt mechanism supporting multiple interrupt vectors, useful for distributing device queues across CPUs.

---

## Q10. What is NAPI?

Linux networking's mechanism for combining interrupt-driven notification with polling/batching to handle high packet rates efficiently.

---

# 52. What You Must Master for Senior Interviews

Priority order:

```text
★★★★★  Interrupt flow
★★★★★  Interrupt vs process context
★★★★★  Why IRQ handlers cannot sleep
★★★★★  Top half / deferred processing
★★★★★  Workqueues
★★★★★  Spinlocks and IRQ-safe locking
★★★★★  DMA + interrupt completion
★★★★★  MSI/MSI-X
★★★★★  Interrupt affinity
★★★★★  Interrupt storms
★★★★★  /proc/interrupts
★★★★☆  NAPI
★★★★☆  Wait queues
★★★★☆  Completions
★★★☆☆  Softirqs
★★★☆☆  Tasklets
```

---

# 53. Final Connection

Device drivers and interrupts should be understood together:

```text
                    DEVICE DRIVER
                         |
        +----------------+----------------+
        |                |                |
       MMIO             DMA              IRQ
        |                |                |
        |                |                v
        |                |         Interrupt Handler
        |                |                |
        |                +----------------+
        |                         |
        v                         v
   Configure                  Completion
   Hardware                       |
        |                         v
        +------------------> Wake/Notify
                                  |
                                  v
                              User Space
```

The most important senior-level idea is:

> **A high-performance Linux driver normally configures hardware through MMIO, transfers bulk data through DMA, receives completion notifications through interrupts, performs only minimal work in hard IRQ context, and defers heavier processing to an appropriate context.**
