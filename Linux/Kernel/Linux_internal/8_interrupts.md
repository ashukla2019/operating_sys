# Linux Interrupts

## 1. What Is an Interrupt?
An interrupt is a mechanism by which hardware or software requests CPU attention. Without interrupts, the CPU would need to continuously check devices (Check NIC, Check Disk, Check UART, Check Timer, Check USB, Repeat...) — inefficient. With interrupts:
```
CPU executes normal work --> Hardware event --IRQ--> CPU --> Interrupt Handler
```
The CPU can perform other work until the device actually needs attention.

## 2. Why Are Interrupts Needed?
Consider a NIC — without interrupts, the CPU must repeatedly ask "Is packet available?" (polling). With interrupts:
```
CPU doing other work --> NIC: packet arrives --IRQ--> CPU --> Network Driver
```
The CPU is notified only when necessary.

## 3. Basic Interrupt Flow
```
Hardware --Interrupt Request--> Interrupt Controller --> CPU --> Kernel Interrupt Entry --> Interrupt Handler --> Driver
```
The exact hardware details vary by architecture.

## 4. IRQ
IRQ means **Interrupt Request** — a device can generate one. Examples: NIC → packet received, NVMe → I/O completed, UART → data received, Timer → timer expired, GPU → command completed, USB → transfer completed.

## 5. Interrupt Controller
The CPU normally does not directly manage every device interrupt — an interrupt controller receives interrupt requests and routes them appropriately:
```
Devices (NIC, NVMe, UART) --> Interrupt Controller --> CPU
```
On modern systems there can be multiple interrupt-controller layers.

## 6. Interrupt Number
Linux identifies interrupts using IRQ numbers, inspectable via `cat /proc/interrupts`:
```
           CPU0       CPU1
  40:       100         50   NIC
  41:        20         30   NVMe
```
The exact output depends on the machine.

## 7. /proc/interrupts
An extremely useful debugging interface (`cat /proc/interrupts`) showing IRQ number, interrupt count, per-CPU interrupt distribution, interrupt controller information, and device/driver association. This can help identify interrupt imbalance, interrupt storms, CPU affinity problems, and unexpected interrupt activity.

---

## 8. Interrupt Handler
A driver registers an interrupt handler, e.g. `request_irq(irq, handler, ...);`. The handler is invoked when the corresponding interrupt occurs: `Device --> IRQ --> handler()`.

## 9. Interrupt Handler Responsibilities
An interrupt handler should normally perform only urgent work:
1. Determine interrupt source
2. Acknowledge/clear interrupt
3. Read minimal device status
4. Capture necessary information
5. Schedule deferred processing
6. Return quickly

Avoid doing large amounts of work directly in hard interrupt context.

## 10. Why Must Interrupt Handlers Be Fast?
A long handler leaves the CPU unavailable for other work, which can delay networking, audio, storage, real-time workloads, and system responsiveness. So: do minimal work in the handler, defer expensive work.

---

## 11. Hard IRQ Context
The immediate interrupt handler runs in interrupt context:
```
Normal execution --> Hardware IRQ --> Hard IRQ context --> Return --> Normal execution
```
Important rule: code executing in hard interrupt context must not sleep.

## 12. Why Can't IRQ Handlers Sleep?
Sleeping means the current execution waits for something while the scheduler chooses another task. But an interrupt handler is not running as a normal schedulable process. Therefore, generally avoid `mutex_lock()`, `kmalloc(..., GFP_KERNEL)`, blocking I/O, and `wait_event()` in hard IRQ context.

## 13. Interrupt Context vs Process Context
Critical distinction.
**Process Context** — `System call --> Driver`; the driver executes on behalf of a process and can generally sleep, block, use mutexes, and perform blocking operations.
**Interrupt Context** — `Hardware --> IRQ Handler`; the handler cannot sleep, must execute quickly, and uses atomic/IRQ-safe synchronization.

---

## 14. Top Half
Historically, interrupt processing was divided into Top Half and Bottom Half. The top half executes immediately when the interrupt occurs, typically: acknowledge interrupt, read status, save minimal information, schedule deferred work — then returns quickly.

## 15. Bottom Half
The bottom half handles work that doesn't need to happen immediately: `Hardware --> Top Half --> Bottom Half --> Longer Processing`. Linux provides several mechanisms for deferred work.

## 16. Deferred Interrupt Processing
Important mechanisms: Softirqs, Tasklets, Workqueues, Threaded IRQs. Understand the differences rather than memorizing old APIs.

## 17. Softirq
Softirqs are a mechanism for deferred kernel work — they run in an atomic context and therefore cannot sleep: `Hard IRQ --> Softirq --> Deferred processing`. Used by important kernel subsystems, including networking.

## 18. Tasklets
Tasklets were historically used for deferred interrupt processing: `Hard IRQ --> Tasklet --> Deferred work`. Important interview point: tasklets cannot sleep. For modern driver development, workqueues and threaded IRQs are often more relevant.

## 19. Workqueue
A workqueue executes deferred work in process context: `IRQ --> Schedule Work --> Workqueue --> Worker Thread --> Driver Processing`. Because it runs in process context, the work can generally sleep when appropriate.

## 20. Threaded IRQ
Linux supports threaded interrupt handlers: `Hardware IRQ --> Primary IRQ Handler --> IRQ Thread --> Longer Processing`. Useful when interrupt processing requires operations that can sleep.

## 21. Comparing Deferred Mechanisms
| Mechanism | Can Sleep? | Typical Use |
|---|---|---|
| Hard IRQ | No | Immediate interrupt handling |
| Softirq | No | High-performance deferred kernel work |
| Tasklet | No | Legacy/simple deferred work |
| Workqueue | Yes | Deferred process-context work |
| Threaded IRQ | Yes in threaded part | Device interrupt processing |

The exact kernel execution context and rules matter more than memorizing the table.

---

## 22. Interrupt Handler Example
```c
irqreturn_t my_irq_handler(int irq, void *data)
{
    struct device_data *dev = data;

    /* Read device status */
    status = readl(dev->base + STATUS);

    /* Acknowledge interrupt */
    writel(status, dev->base + IRQ_ACK);

    /* Defer expensive work */
    schedule_work(&dev->work);

    return IRQ_HANDLED;
}
```
The important architecture: `IRQ --> Read status, Acknowledge, Schedule work --> Return quickly`.

## 23. IRQ_RETURN Values
An interrupt handler commonly returns `IRQ_HANDLED` when it handled the interrupt, or `IRQ_NONE` when the interrupt was not from that device:
```
IRQ --> Handler --> Device caused it? YES --> IRQ_HANDLED / NO --> IRQ_NONE
```
This is particularly relevant for shared interrupts.

## 24. Shared Interrupts
Multiple devices can sometimes share an interrupt line:
```
Device A, Device B, Device C ----> IRQ ----> CPU
```
The handlers need to determine whether their device generated the interrupt (Handler A checks device A, etc.). If a handler did not handle the interrupt, `IRQ_NONE` can be returned.

## 25. Interrupt Storm
An interrupt storm occurs when a device generates interrupts excessively — the CPU spends too much time handling interrupts. Symptoms: high CPU usage, poor application performance, high interrupt latency, system instability.

## 26. Causes of Interrupt Storms
Possible causes: interrupt not acknowledged, interrupt status not cleared, hardware malfunction, driver bug, incorrect interrupt configuration, device repeatedly reporting the same event. Debug with `cat /proc/interrupts` and driver logs/tracing.

---

## 27. Interrupt Affinity
On multicore systems, interrupts can be routed to particular CPUs, e.g. `NIC IRQ --> CPU 2`, or per-queue: `RX queue 0 → CPU 0`, `RX queue 1 → CPU 1`, etc. Important for high-performance networking and storage.

## 28. /proc/irq
Linux exposes interrupt configuration through `/proc/irq/<IRQ>/` — information can include affinity, interrupt controller information, and statistics, depending on kernel configuration.

## 29. SMP and Interrupts
On a multicore system, the Interrupt Controller routes to CPU0/CPU1/CPU2/etc. — the kernel must coordinate interrupt processing across CPUs, which introduces concurrency issues.

## 30. Interrupts and Locking
If a driver shares data between process context and interrupt context, a normal mutex may not be appropriate because the interrupt handler cannot sleep. The driver may need an IRQ-safe locking strategy:
```c
spin_lock_irqsave(&lock, flags);
...
spin_unlock_irqrestore(&lock, flags);
```
A common pattern when the same lock can be accessed from interrupt and process context.

## 31. Why spin_lock_irqsave()?
If process context holds a lock and an IRQ arrives whose handler tries the same lock, the CPU can deadlock (the handler spins waiting for a lock held by the interrupted code). Disabling local interrupts while holding the lock prevents this specific re-entry scenario:
```
Process Context --> Disable local IRQs --> Acquire spinlock --> Critical Section
--> Release lock --> Restore IRQ state
```
The exact locking strategy must match where the lock is used.

## 32. Spinlock in Interrupt Context
A spinlock is appropriate when the critical section is short and code cannot sleep:
```
IRQ Handler --> spin_lock() --> update shared state --> spin_unlock()
```
Do not perform long operations while holding a spinlock.

---

## 33. Interrupt Latency
Interrupt latency is the time between the interrupt occurring and the handler starting: `IRQ occurs --latency--> Handler begins`. Low latency is important for real-time systems, audio, control systems, and high-performance devices.

## 34. Interrupt Processing Time
Two separate concepts: **Interrupt latency** (`IRQ → handler starts`) and **Interrupt handling time** (`Handler starts → handler completes`). A system can have low latency but long handler execution, or the reverse.

## 35. Interrupt Coalescing
High-speed devices can reduce interrupt frequency by combining multiple events. Without coalescing: each packet triggers an IRQ. With coalescing: multiple packets → one IRQ. Benefits: lower interrupt overhead, higher throughput. Trade-off: potentially higher latency. Widely used in NICs and other high-throughput devices.

---

## 36. MSI
PCI/PCIe devices can use **Message Signaled Interrupts** — instead of relying on a traditional physical interrupt line, the device generates an interrupt through a memory transaction mechanism: `PCIe Device --MSI--> Interrupt System --> CPU`.

## 37. MSI-X
MSI-X supports multiple interrupt vectors — especially useful for devices with multiple queues, e.g. `RX Queue 0 → IRQ 0`, `RX Queue 1 → IRQ 1`, etc. These can be distributed across CPUs.

## 38. NIC Interrupt Flow
A modern network receive path:
```
Network Packet --> NIC --DMA--> RX Ring --> Interrupt --> Network Driver
--> Deferred/NAPI Processing --> Network Stack --> Socket --> Application
```
This connects interrupts with DMA, networking, and scheduling.

## 39. NAPI
Linux networking uses NAPI to reduce interrupt overhead under high packet rates. Low traffic: `Interrupt --> Process packets`. High traffic: `Interrupt --> Disable/reduce further RX interrupts --> Polling --> Process batch of packets --> Re-enable interrupts`. The goal is to combine interrupt-driven notification with polling for efficient packet processing.

## 40. Why NAPI?
Under extremely high traffic, per-packet interrupts become excessive overhead. NAPI allows `IRQ --> Poll many packets --> Batch processing`, improving throughput and reducing interrupt overhead.

---

## 41. Storage Interrupt Example
Consider NVMe:
```
Application --> Filesystem --> Block Layer --> NVMe Driver --> NVMe Controller
--DMA--> Memory --> Completion --> MSI-X Interrupt --> Driver --> Complete I/O
```
This is a very important senior Linux/storage flow.

## 42. Interrupt + DMA Relationship
A common hardware pattern:
```
Driver --Configure DMA--> Device --DMA transfer--> RAM --Transfer complete--> IRQ --> Driver
```
The CPU is not required to copy every byte.

## 43. Interrupt + Wait Queue
An application waiting for device data: `Application --> read() --> Wait Queue --> Sleep`. Device receives data: `Hardware --> IRQ --> Driver --> Wake Up --> Application`. The application becomes runnable again.

## 44. Interrupt + Completion
Another common pattern: `Process Context --> Start hardware operation --> wait_for_completion() --> Sleep`. Hardware finishes: `Hardware --> IRQ --> Driver --> complete() --> Wake process`. A clean synchronization model for device operations.

## 45. Interrupt Safety Rules
In hard interrupt context:
- **DO:** keep handler short; use atomic/IRQ-safe synchronization; acknowledge interrupt; schedule deferred work; update protected state
- **DON'T:** sleep; block; take a mutex that may sleep; perform long operations; perform unnecessary allocations

---

## 46. Common Interrupt Bugs
1. **Interrupt not cleared** — `Device --> IRQ --> Handler` fails to clear it, so the interrupt remains active and fires again and again → interrupt storm.
2. **Sleeping in IRQ** — `IRQ Handler --> Blocking operation` → invalid context, can produce warnings or crashes.
3. **Race with shared state** — CPU 0 modifies state while the IRQ reads it concurrently; without proper synchronization, the interrupt may observe inconsistent data.
4. **Excessive handler work** — `IRQ --> Huge processing --> High latency`; move expensive work to an appropriate deferred context.

---

## 47. Debugging Interrupt Problems
First check `cat /proc/interrupts`, looking for unexpectedly high interrupt counts, one CPU receiving all interrupts, interrupt count not increasing, or interrupt count increasing too rapidly. Then inspect `dmesg`, `/sys`, `/proc/irq`, `ftrace`, tracepoints, `perf`.

## 48. Interrupt Debugging Example
Suppose CPU usage is 100%. `cat /proc/interrupts` shows `IRQ 45: CPU0 = 50000000, CPU1 = 10` — suspicion: interrupt storm. Next investigate: which device owns IRQ 45? Is the interrupt being acknowledged? Is the device continuously generating events? Is IRQ affinity correct? Is the driver stuck?

---

## 49. Senior Interview Scenario
**Question:** A device driver causes CPU usage to reach 100%. How would you debug it?

**Answer structure:**
1. Check `/proc/interrupts`
2. Identify rapidly increasing IRQ
3. Identify device/driver
4. Check whether interrupt is being acknowledged
5. Check driver logs
6. Check IRQ affinity
7. Check for interrupt storm
8. Inspect handler/deferred work
9. Trace interrupt activity if necessary
10. Check device/hardware state

This is much stronger than simply saying "I would check the CPU."

---

## 50. Interrupt Mental Model
Memorize:
```
                    HARDWARE
                        |
                       IRQ
                        v
                INTERRUPT CONTROLLER
                        |
                       CPU
                        |
                 HARD IRQ HANDLER
                        |
             +----------+----------+
        Immediate work       Deferred work
                                   |
                   +---------------+---------------+
                Softirq        Workqueue      IRQ Thread
                   +---------------+---------------+
                                   |
                              DRIVER STATE
                                   |
                              USER SPACE
```

---

## 51. Important Interview Questions

**Q1. What is an interrupt?**
A mechanism that allows hardware/software to request CPU attention asynchronously.

**Q2. Why use interrupts instead of polling?**
Interrupts allow the CPU to perform useful work until an event occurs, reducing unnecessary CPU usage.

**Q3. Can an interrupt handler sleep?**
No, hard interrupt context cannot sleep.

**Q4. What is a bottom half?**
A mechanism for deferring interrupt-related processing so the hard interrupt handler can return quickly.

**Q5. Softirq vs workqueue?**
Softirq → atomic context, cannot sleep. Workqueue → process context, can generally sleep.

**Q6. What is an interrupt storm?**
A situation where interrupts occur excessively, consuming significant CPU time.

**Q7. How do you detect an interrupt storm?**
Start with `cat /proc/interrupts` and identify IRQs whose counters are increasing abnormally fast.

**Q8. What is interrupt affinity?**
The CPU or set of CPUs to which an interrupt can be routed.

**Q9. What is MSI-X?**
A PCI/PCIe interrupt mechanism supporting multiple interrupt vectors, useful for distributing device queues across CPUs.

**Q10. What is NAPI?**
Linux networking's mechanism for combining interrupt-driven notification with polling/batching to handle high packet rates efficiently.

---

## 52. What You Must Master for Senior Interviews
Priority order:
```
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

## 53. Final Connection
Device drivers and interrupts should be understood together:
```
                    DEVICE DRIVER
                         |
        +----------------+----------------+
       MMIO             DMA              IRQ
        |                |                |
        |                |         Interrupt Handler
        |                |                |
        |                +----------------+
        |                         |
    Configure                 Completion
    Hardware                      |
        |                         v
        +------------------> Wake/Notify
                                  |
                              User Space
```

The most important senior-level idea: **a high-performance Linux driver normally configures hardware through MMIO, transfers bulk data through DMA, receives completion notifications through interrupts, performs only minimal work in hard IRQ context, and defers heavier processing to an appropriate context.**
