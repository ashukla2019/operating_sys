# GPU / Graphics Driver Interview Notes — OpenGL + Linux DRM/KMS


> **Hands-on experience focus:** OpenGL, Mesa, libdrm, Linux DRM/KMS, GPU driver flow, graphics memory, command submission, synchronization, interrupts, and debugging.

> **Purpose:** Senior-level GPU / Graphics Driver interview preparation, especially for Linux graphics stacks where you have hands-on experience with the architecture.
>
> **Source basis:** The original document covered GPU fundamentals, DRM/KMS, Mesa, OpenGL, shaders, rasterization, framebuffer concepts, and basic OpenGL setup. The material below preserves those fundamentals and adds the driver-oriented topics needed for deeper interviews.

---

# Overall Architecture — The Architecture You Worked With

This is the **primary diagram to remember in the interview**. It keeps the architecture from your original document and expands it into the full Linux graphics-driver path.

```text
                         +----------------------+
                         |      APPLICATION     |
                         +----------+-----------+
                                    |
                           OpenGL API
                                    |
                                    v
                    +---------------+----------------+
                    |       USERSPACE GRAPHICS       |
                    |                                |
                    |   Mesa / GL /  Driver   |
                    +---------------+----------------+
                                    |
                               libdrm / ioctl
                                    |
                                    v
                    +---------------+----------------+
                    |          LINUX DRM             |
                    |                                |
                    |  DRM Core + GPU Driver        |
                    +-----------+----------+---------+
                                |          |
                    GPU path    |          |   Display path
                                |          |
                                v          v
                     +----------+--+   +--+-----------+
                     | GPU / GPU   |   |     KMS     |
                     |   Memory   |   |             |
                     +------+-----+   | Plane       |
                            |         | CRTC        |
                            |         | Encoder     |
                            |         | Connector   |
                            |         +------+------+
                            |                |
                            |                v
                            |      Display Controller
                            |                |
                            |          HDMI / DP / eDP
                            |                |
                            |                v
                            |             DISPLAY
                            |
                            +--> Command Submission
                            +--> GPU MMU / VM
                            +--> DMA / IOMMU
                            +--> Scheduler
                            +--> IRQ / Completion
```

### The key separation

```text
                    LINUX DRM
                       |
          +------------+------------+
          |                         |
          v                         v
    GPU / Rendering               KMS
          |                         |
          v                         v
     GPU + Memory          Display Controller
                                    |
                                    v
                                  Screen
```

**Interview sentence:**

> "In the Linux graphics stack I worked with, DRM provides the kernel-side graphics infrastructure and GPU interaction, while KMS handles the display pipeline and mode setting. Mesa/libdrm provide the userspace side of the graphics path."

---

# Architecture Diagram — Rendering Path

```text
Application
    |
    | glDraw*() /  command
    v
OpenGL
    |
    v
Mesa
    |
    | generate / prepare GPU work
    v
Userspace GPU Driver
    |
    | ioctl()
    v
DRM
    |
    v
Kernel GPU Driver
    |
    +--> Allocate / map buffers
    +--> Build command submission
    +--> Queue work
    +--> Track dependencies
    |
    v
GPU
    |
    +--> Vertex Processing
    +--> Primitive Assembly
    +--> Rasterization
    +--> Fragment Processing
    +--> Depth / Blend
    |
    v
Render Target / Framebuffer
```

---

# Architecture Diagram — Display Path

This is the **KMS side** of the architecture.

```text
              Rendered Buffer
                    |
                    v
               DRM Framebuffer
                    |
                    v
                  Plane
                    |
                    v
                  CRTC
                    |
                    v
                 Encoder
                    |
                    v
                Connector
                    |
          +---------+---------+
          |         |         |
        HDMI        DP       eDP
          |         |         |
          +---------+---------+
                    |
                    v
                 DISPLAY
```

### KMS Objects

```text
Plane
  |
  | selects/combines framebuffer source
  v
CRTC
  |
  | generates display timing / scanout
  v
Encoder
  |
  | converts output
  v
Connector
  |
  v
Physical Display Link
```

---

# Architecture Diagram — GPU Memory Path

```text
                    CPU
                     |
             CPU Virtual Address
                     |
                     v
                 CPU MMU
                     |
                     v
                System RAM
                     ^
                     |
                     | DMA
                     |
              +------+------+
              |    IOMMU    |
              +------+------+
                     ^
                     |
              Device Address
                     ^
                     |
                   GPU
                     |
              GPU Virtual Address
                     |
                  GPU MMU
                     |
          +----------+----------+
          |                     |
          v                     v
        VRAM                System RAM
```

### Important interview point

```text
CPU virtual address != GPU virtual address
CPU physical address != necessarily device DMA address
```

The driver establishes the mappings required for the GPU/device to access memory safely.

---

# Architecture Diagram — DMA-BUF / Buffer Sharing

```text
                 +-------------+
                 |   Producer  |
                 | GPU / Camera|
                 +------+------+
                        |
                        | creates buffer
                        v
                 +------+------+
                 |  DMA-BUF    |
                 | shared fd   |
                 +------+------+
                        |
              +---------+---------+
              |                   |
              v                   v
        +-----+------+      +-----+------+
        |    GPU     |      |   Display  |
        | / Device A |      | / Device B |
        +------------+      +------------+
```

The key idea:

> DMA-BUF allows Linux devices/subsystems to share a buffer without requiring unnecessary CPU copies.

---

# Architecture Diagram — Command Submission

```text
Application
    |
    v
OpenGL
    |
    v
Mesa / Userspace Driver
    |
    v
Command Buffer
    |
    v
DRM ioctl
    |
    v
Kernel GPU Driver
    |
    +--> Validate / prepare
    +--> Resolve resources
    +--> Handle dependencies
    |
    v
DRM Scheduler / Driver Queue
    |
    v
GPU Engine
    |
    +--> 3D
    +--> Compute
    +--> Copy / DMA
    |
    v
GPU Executes Commands
```

---

# Architecture Diagram — CPU/GPU Synchronization

```text
CPU
 |
 | submit Job A
 v
GPU Queue
 |
 v
GPU executes Job A
 |
 | completion
 v
Fence / Sync Object
 |
 +------------------------+
 |                        |
 v                        v
CPU wait/observe      Job B dependency
                         |
                         v
                    GPU Job B
```

### Why synchronization is needed

CPU and GPU are asynchronous:

```text
CPU:  submit -----------------------------> continue
GPU:           execute -------- complete
```

Therefore:

```text
submit != complete
```

---

# Architecture Diagram — GPU Interrupt / Completion

```text
                +-------------+
                |     GPU     |
                +------+------+ 
                       |
             completion / fault
                       |
                    IRQ/MSI
                       |
                       v
                +------+------+
                | CPU / IRQ   |
                | handling    |
                +------+------+
                       |
                       v
                 GPU Driver
                       |
          +------------+------------+
          |            |            |
          v            v            v
      Mark job      Signal       Handle
      complete      fence        fault
                       |
                       v
                  Wake waiter /
                  allow next work
```

---

# Architecture Diagram — GPU Hang Debugging

```text
                  Application
                       |
                       v
                 OpenGL
                       |
                       v
                     Mesa
                       |
                       v
                    libdrm
                       |
                     ioctl
                       |
                       v
                  DRM / Driver
                       |
          +------------+-------------+
          |            |             |
          v            v             v
       Memory      Scheduler       IRQ
          |            |             |
          +------------+-------------+
                       |
                       v
                      GPU
                       |
                 +-----+-----+
                 |           |
              Works       Hangs
                 |           |
                 v           v
             Completion   Fault/Hang
                             |
                             v
                         dmesg/logs
                             |
                             v
                    trace / debug tools
```

### Debugging decision path

```text
Did userspace generate valid work?
        |
       yes
        v
Did ioctl reach the driver?
        |
       yes
        v
Are buffers mapped correctly?
        |
       yes
        v
Was the command submitted?
        |
       yes
        v
Did the GPU execute it?
        |
       no
        v
Check:
- GPU fault
- IOMMU fault
- scheduler
- command stream
- firmware
- hardware
- IRQ/completion path
```

---

# One-Page Interview Mental Model

When the interviewer asks **"Explain the graphics driver architecture you worked on"**, use this order:

```text
             APPLICATION
                  |
             OpenGL
                  |
                 Mesa
                  |
               libdrm
                  |
                ioctl
                  |
                 DRM
                  |
       +----------+----------+
       |                     |
   GPU DRIVER               KMS
       |                     |
       |              Plane/CRTC/
       |              Encoder/Connector
       |
       +--> Memory
       +--> DMA/IOMMU
       +--> DMA-BUF
       +--> Command submission
       +--> Scheduler
       +--> Sync/Fences
       +--> IRQ
       |
       v
      GPU
       |
       +--> render
       |
       v
   Framebuffer
       |
       v
      KMS
       |
       v
 Display Controller
       |
       v
 HDMI / DP / eDP
       |
       v
    DISPLAY
```

This is the **core architecture diagram to practice drawing on a whiteboard**.

---

# Chapter 1 – GPU & Rendering Fundamentals

## 1. CPU vs GPU

### CPU
- General-purpose processor.
- Optimized for low-latency execution and complex control flow.
- Handles OS, applications, I/O, and general computation.

### GPU
- Specialized processor designed for highly parallel workloads.
- Particularly effective for graphics and other data-parallel workloads.
- Executes many similar operations in parallel.

**Interview point:**

> A CPU is optimized primarily for general-purpose, low-latency computation, while a GPU is optimized for massive parallel throughput.

---

# 1.1 Graphics Architecture

A simplified Linux graphics architecture is:

```text
Application
    |
    | OpenGL
    v
Userspace Graphics Stack
    |
    | Mesa / libGL /  loader
    v
libdrm
    |
    | ioctl()
    v
Linux DRM
    |
    +----------------------+
    |                      |
    v                      v
GPU Driver                KMS
    |                      |
    v                      v
GPU + GPU Memory      Display Controller
                           |
                           v
                     HDMI / DP / eDP
                           |
                           v
                         Display
```

The original document identifies the important separation:

```text
DRM -> GPU / Graphics Memory
KMS -> Display Controller -> Screen
```

DRM is the Linux kernel graphics subsystem responsible for interfacing with GPUs and graphics memory/cards.

KMS means Kernel Mode Setting and moves display configuration into the kernel.

---

# 1.2 Rendering Pipeline

A simplified graphics pipeline:

```text
Vertex Data
    |
    v
Vertex Shader
    |
    v
Primitive Assembly
    |
    v
Rasterization
    |
    v
Fragment Shader
    |
    v
Depth / Stencil / Blending
    |
    v
Framebuffer
    |
    v
Display
```

## Vertex Shader

Processes vertices.

Typical responsibilities:

- Transform vertex position.
- Process vertex attributes.
- Pass data to later stages.

Examples of attributes:

```text
position
color
normal
texture coordinates
```

## Primitive Assembly

Groups vertices into primitives such as:

```text
Triangle
Line
Point
```

## Rasterization

Converts geometric primitives into fragments/pixel candidates.

```text
Triangle
   |
   v
Rasterization
   |
   +--> Fragment 1
   +--> Fragment 2
   +--> Fragment 3
   +--> ...
```

## Fragment Shader

Processes fragments and determines values such as:

- Color
- Depth
- Alpha

## Framebuffer

OpenGL does not directly "draw to the physical screen."

Rendering normally produces data in a framebuffer. The display subsystem later scans the appropriate buffer to the display.

---

# 1.3 Culling

Culling removes primitives that do not need to be processed.

Example:

```text
Back-facing triangle
       |
       v
     Culling
       |
       X
```

This reduces unnecessary GPU work.

---

# 1.4 Shaders

Shaders are programs executed by GPU shader hardware.

Common stages:

```text
Vertex Shader
      |
      v
Fragment Shader
```

GLSL is a shader language used with OpenGL.

---

# 1.5 Double Buffering

Typical concept:

```text
Front Buffer  ---> Display
Back Buffer   ---> GPU renders here

After rendering:

Front <----> Back
```

Benefits:

- Avoids displaying partially rendered frames.
- Reduces visible tearing/artifacts.

---

# 1.6 Depth Buffer

A depth buffer stores depth information for fragments.

```text
Fragment A -> depth 0.3
Fragment B -> depth 0.8

A is closer
=> A can be visible
```

Depth testing prevents hidden surfaces from incorrectly appearing in front.

---

# 1.7 OpenGL Context

An OpenGL context contains the state required for OpenGL rendering.

Typical setup includes:

```text
Create context
    |
    v
Initialize GL state
    |
    v
Create resources
    |
    v
Render
    |
    v
Swap buffers
```

The original document also demonstrates initialization of depth testing and face culling.

---

# Chapter 2 – Linux DRM / KMS Internals

# 2.1 DRM

DRM = Direct Rendering Manager.

It is a Linux kernel subsystem providing infrastructure for GPU access.

High-level structure:

```text
Userspace
   |
   | ioctl()
   v
DRM Core
   |
   +---- GPU Driver
   |
   +---- Memory Management
   |
   +---- Synchronization
   |
   +---- Scheduling
   |
   +---- KMS
```

---

# 2.2 libdrm

`libdrm` is a userspace library that provides access to DRM functionality through the kernel's ioctl interface.

Typical flow:

```text
Application / Mesa
       |
       v
     libdrm
       |
       | ioctl()
       v
   DRM kernel
       |
       v
   GPU driver
```

Important distinction:

```text
libdrm = userspace helper/library

DRM = kernel subsystem
```

---

# 2.3 ioctl()

Graphics applications normally do not directly access GPU hardware registers from userspace.

Instead:

```text
Userspace
    |
    | ioctl(fd, command, data)
    v
DRM device
    |
    v
DRM / GPU driver
    |
    v
Hardware
```

Interview question:

**Why ioctl?**

Because graphics drivers expose many device-specific operations and structured commands that do not map naturally to simple read/write operations.

---

# 2.4 DRM Device

A DRM device is commonly exposed through:

```text
/dev/dri/
```

Typical nodes include:

```text
/dev/dri/card0
/dev/dri/renderD128
```

Conceptually:

```text
card node
    |
    +--> display/KMS capable operations

render node
    |
    +--> rendering/GPU operations
```

Render nodes are particularly useful when an application needs GPU rendering without requiring display-control privileges.

---

# 2.5 KMS

KMS = Kernel Mode Setting.

KMS handles display configuration from the kernel.

Responsibilities include:

- Display modes.
- Connectors.
- CRTCs.
- Planes.
- Framebuffers.
- Display pipeline configuration.

---

# 2.6 KMS Architecture

Important objects:

```text
Framebuffer
     |
     v
   Plane
     |
     v
   CRTC
     |
     v
  Encoder
     |
     v
 Connector
     |
     v
 HDMI / DP / eDP
```

## Connector

Represents an output connection.

Examples:

```text
HDMI
DisplayPort
eDP
```

## Encoder

Converts the CRTC output into a format suitable for the connector.

## CRTC

A display pipeline engine responsible for scanning framebuffer content and generating display timing.

## Plane

A framebuffer source that can be composed by the display hardware.

Examples:

```text
Primary plane
Cursor plane
Overlay plane
```

---

# 2.7 DRM Framebuffer

A DRM framebuffer describes how display hardware should interpret a memory buffer.

It includes concepts such as:

```text
width
height
pixel format
memory layout
buffer handle
```

The framebuffer is not necessarily the physical memory allocation itself. It describes the displayable buffer.

---

# 2.8 Atomic Modesetting

Modern KMS uses atomic operations.

Instead of changing display state one object at a time:

```text
Plane
CRTC
Connector
Mode
```

a set of changes can be validated and committed together.

Conceptually:

```text
Build new state
      |
      v
Validate
      |
      v
Atomic Commit
      |
      v
Hardware Update
```

Advantages:

- Consistent state transitions.
- Avoids partially applied display configurations.
- Useful for complex multi-plane/multi-display systems.

---

# 2.9 DRM Memory Management

GPU drivers need to manage buffers used by:

- GPU
- CPU
- Display controller
- Other devices

Important concepts:

```text
GEM
TTM
Buffer Object
GPU Virtual Address
DMA-BUF
PRIME
```

---

# 2.10 GEM

GEM = Graphics Execution Manager.

GEM provides common infrastructure for managing graphics memory objects in DRM.

Conceptually:

```text
Userspace
    |
    | handle
    v
GEM object
    |
    v
GPU memory / system memory
```

The exact memory-management implementation is driver-dependent.

---

# 2.11 TTM

TTM = Translation Table Maps.

TTM is a more general DRM memory-management framework used by some GPU drivers.

It supports concepts such as:

- GPU memory placement.
- System memory.
- Migration.
- Eviction.
- Mapping.

Do not claim that every modern driver uses TTM; the memory-management implementation is GPU-driver dependent.

---

# Chapter 3 – Mesa + OpenGL Driver Stack


# Chapter 4 – GPU Memory / DMA / IOMMU / DMA-BUF

# 4.1 Why GPU Memory Management Matters

GPU workloads use large buffers:

```text
Vertex buffers
Index buffers
Textures
Render targets
Framebuffers
Command buffers
```

The GPU must be able to address these buffers efficiently.

---

# 4.2 CPU Virtual Address vs GPU Virtual Address

A CPU pointer and a GPU virtual address are not necessarily the same.

Conceptually:

```text
CPU Virtual Address
        |
        v
      CPU MMU
        |
        v
   Physical Memory


GPU Virtual Address
        |
        v
     GPU MMU
        |
        v
   Physical Memory
```

This separation allows the GPU driver to manage GPU address spaces.

---

# 4.3 GPU Virtual Memory

A modern GPU commonly has virtual address spaces.

Conceptually:

```text
GPU VA
  |
  | GPU page tables
  v
Physical pages
  |
  v
VRAM / System RAM
```

Benefits:

- Process isolation.
- Flexible memory placement.
- Large virtual address spaces.
- Protection.
- Resource management.

---

# 4.4 DMA

DMA = Direct Memory Access.

DMA allows a device to transfer data to/from memory without requiring the CPU to copy every byte.

Example:

```text
GPU / Device
     |
     | DMA
     v
System Memory
```

Without DMA:

```text
Device -> CPU -> Memory
```

With DMA:

```text
Device ------------> Memory
```

The CPU configures the transfer and the device performs it.

---

# 4.5 DMA Mapping

The driver prepares memory so that the device can access it.

Conceptually:

```text
CPU memory
    |
    v
DMA mapping
    |
    v
DMA address
    |
    v
Device
```

The DMA address visible to the device is not necessarily the CPU physical address.

---

# 4.6 IOMMU

IOMMU = Input/Output Memory Management Unit.

It translates device-visible addresses.

Conceptually:

```text
GPU/device address
       |
       v
     IOMMU
       |
       v
Physical memory
```

IOMMU provides mechanisms for:

- Address translation.
- Isolation.
- Protection.
- Device virtualization.

---

# 4.7 DMA-BUF

DMA-BUF is a Linux framework for sharing buffers between devices/subsystems.

Example:

```text
GPU
 |
 | DMA-BUF
 v
Display Controller
```

Another example:

```text
Camera
   |
   | DMA-BUF
   v
GPU
   |
   | DMA-BUF
   v
Display
```

The important concept is **buffer sharing without unnecessary copying**.

---

# 4.8 PRIME

PRIME enables buffer sharing and GPU/display integration, particularly in systems with multiple GPUs.

Conceptually:

```text
GPU A
 |
 | buffer
 v
DMA-BUF
 |
 v
GPU B / Display
```

This is particularly relevant for hybrid graphics systems.

---

# 4.9 Zero-Copy Concept

A simplified goal:

```text
Producer
   |
   | shared buffer
   v
Consumer
```

instead of:

```text
Producer
   |
   v
CPU copy
   |
   v
Consumer
```

DMA-BUF is one of the mechanisms used to enable efficient buffer sharing across Linux devices/subsystems.

---

# 4.10 Buffer Object Lifecycle

A useful interview model:

```text
Create buffer
     |
     v
Allocate / reserve memory
     |
     v
Map / establish GPU access
     |
     v
GPU uses buffer
     |
     v
Synchronize
     |
     v
Release
```

The exact implementation varies by driver.

---

# Chapter 5 – GPU Command Submission / Sync / IRQ / Debugging

# 5.1 Command Submission

Applications do not normally execute GPU instructions directly.

Instead:

```text
Application
    |
    v
Graphics API
    |
    v
Command Buffer
    |
    v
Driver
    |
    v
GPU Queue / Ring
    |
    v
GPU
```

---

# 5.2 Command Buffer

A command buffer contains work that the GPU should execute.

Conceptually:

```text
Command Buffer
+--------------------+
| Set state          |
| Bind resources     |
| Draw               |
| Compute            |
| Synchronization    |
+--------------------+
```

The driver validates/prepares the submission and sends work to a GPU queue/ring.

---

# 5.3 GPU Queue

A GPU may expose multiple execution queues or engines.

Conceptually:

```text
             GPU
              |
      +-------+-------+
      |       |       |
     3D    Compute   Copy
   Engine   Engine   Engine
```

Actual engines and queue capabilities depend on the GPU architecture.

---

# 5.4 GPU Scheduler

Modern Linux GPU drivers may use DRM scheduling infrastructure.

Conceptually:

```text
Userspace submissions
        |
        v
Driver scheduler
        |
        +---- Job 1
        +---- Job 2
        +---- Job 3
        |
        v
GPU engine
```

Scheduler responsibilities can include:

- Ordering work.
- Tracking dependencies.
- Managing execution.
- Handling multiple clients/jobs.

---

# 5.5 CPU-GPU Synchronization

CPU and GPU execute asynchronously.

Example:

```text
CPU
 |
 | submit
 v
GPU
 |
 | executing...
 |
 +--------------------+
                      |
                      v
                 completion
```

The CPU cannot assume that GPU work has completed immediately after submission.

---

# 5.6 Fence

A fence represents completion/dependency information.

Conceptually:

```text
GPU Job A
    |
    v
  Fence
    |
    v
GPU Job B
```

Job B can wait until Job A reaches the required completion point.

---

# 5.7 DMA Fence / Explicit Synchronization

Linux graphics uses synchronization primitives to coordinate access to shared buffers.

Example:

```text
GPU writes buffer
       |
       v
     Fence
       |
       v
Display reads buffer
```

This prevents consumers from reading a buffer before the producer has finished writing it.

---

# 5.8 Interrupts

GPU completion and error conditions can generate interrupts.

Simplified flow:

```text
GPU
 |
 | interrupt
 v
CPU
 |
 v
Interrupt handler
 |
 v
Driver
 |
 +--> mark job complete
 +--> wake waiters
 +--> process error
 +--> schedule more work
```

Common GPU interrupt events can include:

- Command completion.
- Faults.
- Engine errors.
- Page faults.
- GPU hangs/errors.

The exact interrupt architecture is hardware-dependent.

---

# 5.9 GPU Hang

A GPU hang means GPU execution has stopped progressing as expected.

Possible causes:

```text
Invalid command
Memory fault
Bad synchronization
Hardware issue
Driver bug
Firmware issue
```

---

# 5.10 GPU Hang Debugging

A good debugging approach:

```text
Application
    |
    v
API validation
    |
    v
Mesa / userspace driver
    |
    v
DRM ioctl
    |
    v
Kernel GPU driver
    |
    +--> Command submission
    +--> Memory mappings
    +--> Scheduler
    +--> IRQ
    |
    v
GPU hardware
```

Check systematically:

### 1. Userspace

- API errors.
- Invalid resources.
- Incorrect synchronization.
- Command recording/submission.

### 2. DRM / ioctl

- Correct ioctl.
- Correct handles.
- Buffer state.
- File descriptor/device node.

### 3. Memory

- GPU virtual address mapping.
- DMA mapping.
- IOMMU faults.
- Buffer lifetime.
- Access permissions.

### 4. Command submission

- Queue/engine.
- Command buffer.
- Dependencies.
- Scheduler state.

### 5. Interrupts

- Did the GPU generate completion/error IRQ?
- Did the driver receive it?
- Did the completion state get updated?

### 6. Hardware

- GPU fault.
- Page fault.
- Engine hang.
- Firmware issue.

---

# 5.11 Useful Linux Debugging Tools

For graphics-driver debugging, become comfortable with:

```text
dmesg
journalctl
ls /dev/dri/
lspci
cat /proc/interrupts
cat /sys/kernel/debug/dri/*
```

And general performance/debugging tools:

```text
gdb
perf
ftrace
trace-cmd
strace
```

GPU-specific debugging facilities vary by vendor and driver.

---

# 5.12 PCIe and GPU

For a discrete GPU:

```text
CPU
 |
 | PCIe
 v
GPU
 |
 +---- VRAM
 |
 +---- GPU engines
 |
 +---- Display engines
```

PCIe provides the host/device interconnect.

Important concepts:

```text
PCIe BARs
MMIO
DMA
PCIe interrupts
MSI / MSI-X
PCIe configuration space
```

---

# 5.13 MMIO

MMIO = Memory-Mapped I/O.

The CPU accesses device registers through mapped address ranges.

Conceptually:

```text
CPU
 |
 | read/write
 v
MMIO register
 |
 v
GPU hardware
```

Drivers use MMIO to configure hardware registers.

---

# 5.14 GPU Driver Probe

A simplified Linux PCI GPU driver lifecycle:

```text
PCI device discovered
        |
        v
Driver probe()
        |
        +--> Initialize hardware
        +--> Map BARs / MMIO
        +--> Initialize memory management
        +--> Initialize GPU engines
        +--> Initialize interrupts
        +--> Register DRM device
        |
        v
Device ready
```

---

# 5.15 Complete Graphics Driver Flow

This is the most important architecture to remember for interviews:

```text
                         APPLICATION
                              |
                    +---------+---------+
                    |                   |
                 OpenGL               
                    |                   |
                    v                   v
                  Mesa            Mesa
                    |                   |
                    +---------+---------+
                              |
                           libdrm
                              |
                            ioctl
                              |
                         DRM CORE
                              |
              +---------------+----------------+
              |                                |
             KMS                         GPU DRIVER
              |                                |
              |                    +-----------+-----------+
              |                    |           |           |
              |                 Memory     Scheduler     IRQ
              |                    |           |           |
              |                    +-----+-----+-----------+
              |                          |
              |                     Command Submit
              |                          |
              +--------------------------+
                              |
                             GPU
                              |
                 +------------+------------+
                 |                         |
              GPU Memory              Display Engine
                 |                         |
                 +------------+------------+
                              |
                         Framebuffer
                              |
                             KMS
                              |
                     Display Controller
                              |
                       HDMI / DP / eDP
                              |
                           DISPLAY
```

---

# Senior Interview Questions

## Fundamentals

1. CPU vs GPU?
2. What is a graphics pipeline?
3. Vertex shader vs fragment shader?
4. What is rasterization?
5. What is a framebuffer?
6. Why do we need a depth buffer?
7. What is double buffering?
8. What is culling?

## DRM/KMS

9. What is DRM?
10. DRM vs KMS?
11. What is libdrm?
12. How does ioctl reach a GPU driver?
13. What is a DRM device node?
14. `card0` vs `renderD128`?
15. What is a DRM framebuffer?
16. What are CRTC, plane, encoder and connector?
17. What is atomic modesetting?
18. How does a framebuffer reach HDMI?

## Mesa / API

19. What is Mesa?
20. OpenGL vs ?
21. What happens after an OpenGL draw call?
22. What happens after a  command buffer is submitted?
23. Why does  expose more explicit synchronization?
24. Mesa vs kernel GPU driver?

## Memory

25. What is GEM?
26. What is TTM?
27. What is a buffer object?
28. CPU virtual address vs GPU virtual address?
29. What is GPU virtual memory?
30. What is DMA?
31. What is IOMMU?
32. What is DMA-BUF?
33. What is PRIME?
34. Why is zero-copy important?

## Command / Sync

35. How does GPU command submission work?
36. What is a command buffer?
37. What is a GPU queue/engine?
38. What does a GPU scheduler do?
39. Why is synchronization required between CPU and GPU?
40. What is a fence?
41. How do two devices safely share a buffer?

## Interrupts / Debugging

42. How does a GPU completion interrupt work?
43. What happens when a GPU hangs?
44. How would you debug a GPU hang?
45. How would you debug an IOMMU fault?
46. How would you determine whether the problem is Mesa or the kernel driver?
47. How would you determine whether the GPU actually received the command?
48. What information would you inspect in `dmesg`?
49. What role does PCIe play in a discrete GPU?
50. What is MMIO?

---

# Must-Know Interview Flows

## Flow 1 – Application to GPU

```text
Application
 -> OpenGL
 -> Mesa
 -> libdrm
 -> ioctl
 -> DRM
 -> GPU Driver
 -> Command Queue
 -> GPU
```

## Flow 2 – GPU to Display

```text
GPU renders
 -> Framebuffer
 -> DRM/KMS
 -> Plane
 -> CRTC
 -> Encoder
 -> Connector
 -> HDMI/DP/eDP
 -> Display
```

## Flow 3 – Shared Buffer

```text
Producer
 -> Buffer
 -> DMA-BUF
 -> Consumer
```

## Flow 4 – GPU Memory

```text
GPU Virtual Address
 -> GPU MMU / page tables
 -> IOMMU where applicable
 -> Physical memory
 -> VRAM / System RAM
```

## Flow 5 – GPU Completion

```text
CPU submits job
 -> GPU scheduler
 -> GPU executes
 -> GPU completion
 -> IRQ
 -> Driver
 -> Fence/signaling
 -> CPU/userspace observes completion
```

---

# What You Should Be Able to Explain From Your Real Experience

For a senior graphics-driver interview, do not stop at definitions.

For every component you mention, be ready to answer:

```text
What is it?
Why is it needed?
Where does it run?
Who calls it?
What data does it handle?
How does it interact with the next layer?
How would you debug it?
```

For example:

```text
Application
    |
    | Draw call
    v
Mesa
    |
    | Command generation
    v
libdrm
    |
    | ioctl
    v
DRM
    |
    | Driver operation
    v
GPU Driver
    |
    | Submit command
    v
GPU
```

Then explain the memory and synchronization path:

```text
Buffer
  |
  +--> GPU VA
  |
  +--> DMA mapping
  |
  +--> DMA-BUF sharing
  |
  +--> Fence
  |
  +--> GPU execution
```

---

# Final Interview Positioning

Since the original material represents an architecture you have actually used, use it as the **foundation**, but prepare the additional four chapters deeply.

Your target should be:

```text
GPU Fundamentals
       +
Rendering Pipeline
       +
DRM/KMS
       +
Mesa/OpenGL
       +
GPU Memory
       +
DMA/IOMMU/DMA-BUF
       +
Command Submission
       +
Synchronization
       +
Interrupts
       +
GPU Debugging
```

This combination is much closer to what a **senior Linux GPU/graphics-driver engineer** should be able to discuss.

> **Important:** Keep your real project terminology and actual hardware/driver details when answering interviews. Do not claim experience with a subsystem or GPU architecture that you have not actually used. Use the concepts above to explain and deepen the architecture you genuinely worked with.
---

# Chapter 6 — Senior/Staff GPU Driver Deep-Dive Addendum

> **Scope:** OpenGL + Mesa + libdrm + DRM/KMS + Linux GPU driver interviews. This section intentionally avoids making Vulkan a prerequisite and focuses on the stack aligned with the experience represented by these notes.

## 6.1 Complete End-to-End Rendering Flow

Be able to explain one draw call across every layer:

```text
Application
   |
   | OpenGL API
   v
Mesa / OpenGL userspace driver
   |
   | command/resource preparation
   v
libdrm / DRM userspace interface
   |
   | ioctl()
   v
DRM core
   |
   v
GPU kernel driver
   |
   +--> buffer management
   +--> GPU virtual memory
   +--> command submission
   +--> scheduler
   +--> synchronization
   +--> IRQ handling
   |
   v
GPU engine
   |
   +--> shader execution
   +--> rasterization
   +--> framebuffer
   |
   v
KMS / display pipeline
   |
   v
Display controller / connector / panel
```

### Interview rule

Do not describe Mesa as the kernel driver. Mesa is userspace graphics software; DRM is the kernel graphics framework; the vendor GPU driver implements hardware-specific kernel functionality.

---

## 6.2 DRM Device Nodes: card vs Render Node

Typical nodes:

```text
/dev/dri/card0
/dev/dri/renderD128
```

### Primary distinction

- **card node:** supports display/KMS operations and graphics-device operations according to the driver's capabilities and permissions.
- **render node:** intended for unprivileged render clients without giving them modesetting/control operations.

### Interview question

**Why do render nodes exist?**

They separate rendering access from display-control operations, improving isolation and allowing render clients to access GPU rendering without requiring modesetting privileges.

---

## 6.3 DRM ioctl Dispatch

Understand the boundary rather than only memorizing the word `ioctl`.

```text
Userspace FD
    |
    v
ioctl(fd, request, arg)
    |
    v
DRM ioctl dispatch
    |
    +--> common DRM operation
    |
    +--> driver-specific operation
              |
              v
       GPU driver function
              |
              +--> validate
              +--> resource lookup
              +--> memory handling
              +--> command submission
```

### Senior questions

- Why does the driver validate handles and object state?
- How are userspace resources associated with kernel objects?
- Why should a driver never trust userspace input?
- What happens if a userspace process closes the DRM FD while work is still outstanding?

The answers should lead into **object lifetime, references, fences and cleanup**.

---

## 6.4 GEM vs TTM

### GEM

GEM is a DRM memory-management framework centered around graphics buffer objects and their lifetime/handle management.

### TTM

TTM is a more general GPU memory-management infrastructure designed to handle different memory domains and migration/placement requirements.

### Interview comparison

```text
GEM
 |
 +--> buffer object model
 +--> handles
 +--> common DRM infrastructure

TTM
 |
 +--> memory placement
 +--> VRAM/system-memory management
 +--> migration
 +--> more general GPU memory-management needs
```

Do not answer this as “GEM is new and TTM is old.” Explain the **memory-management model and requirements**.

---

## 6.5 GPU Virtual Memory

A GPU can have an address space distinct from the CPU process address space.

```text
Application / CPU VA
        |
        | userspace resource
        v
Kernel GPU driver
        |
        v
GPU virtual address space
        |
        v
GPU page tables / MMU
        |
        +------> VRAM
        |
        +------> system memory
```

### Important distinctions

```text
CPU virtual address
CPU physical address
GPU virtual address
DMA address / IOVA
```

Never assume these are interchangeable.

---

## 6.6 IOMMU and GPU Memory

A useful mental model:

```text
Device
  |
  | DMA
  v
IOMMU
  |
  | IOVA -> physical mapping
  v
System RAM
```

An IOMMU can provide:

- device address translation
- isolation
- controlled device access to memory
- virtualization support
- fault reporting

### Debugging question

**What could an IOMMU fault tell you?**

It can indicate that a device attempted an invalid/unmapped or unauthorized DMA access. In a GPU driver, investigate buffer lifetime, mappings, address-space state and command validity.

---

## 6.7 DMA-BUF and Buffer Lifetime

DMA-BUF allows buffers to be shared between different device/driver subsystems without unnecessary copies.

```text
Producer
   |
   | DMA-BUF
   v
Shared buffer
   |
   +------> GPU
   |
   +------> display
   |
   +------> camera / codec / other device
```

### Senior-level issue: lifetime

A buffer must remain valid until all users of the buffer have finished accessing it.

```text
Application releases reference
          |
          X  buffer cannot necessarily be freed yet
          |
GPU still executing
          |
          v
Fence signals completion
          |
          v
Final references released
          |
          v
Buffer can be reclaimed
```

This connects **DMA-BUF + references + fences + GPU completion**.

---

## 6.8 Explicit Synchronization / DMA Fence

The essential model is:

```text
Producer
   |
   v
GPU job
   |
   v
Fence created
   |
   +--------------------+
                        |
                        v
                    Consumer
                        |
                  waits/checks fence
                        |
                        v
                  safe buffer access
```

Know the terms:

- `dma_fence`
- fence signaling
- dependency
- implicit synchronization
- explicit synchronization
- buffer lifetime

### Interview question

**Why is a fence not the same thing as a mutex?**

A mutex protects shared software state by mutual exclusion. A fence represents completion/dependency of asynchronous work, often between CPU/device or device/device operations.

---

## 6.9 Atomic KMS

Atomic modesetting treats a display configuration as a state transition.

```text
Userspace
   |
   v
Atomic state
   |
   +--> connector state
   +--> CRTC state
   +--> plane state
   |
   v
Atomic commit
   |
   v
DRM/KMS
   |
   v
Display hardware
```

### Why atomic?

A display update may require several related changes. Atomic state allows the driver to validate and apply a consistent configuration instead of exposing intermediate inconsistent states.

### Senior follow-ups

- What is a plane?
- What is a CRTC?
- What is a connector?
- What is a modeset?
- Why do display pipelines need synchronization?
- What happens when an atomic commit fails?

---

## 6.10 PCIe + BAR + MMIO + IRQ in a GPU

For a discrete GPU:

```text
CPU
 |
 | PCIe
 v
GPU
 |
 +--> BARs / MMIO registers
 +--> VRAM
 +--> engines
 +--> display hardware
```

Typical initialization:

```text
PCI enumeration
      |
      v
GPU driver probe()
      |
      +--> enable PCI device
      +--> discover resources/BARs
      +--> map MMIO
      +--> configure DMA/IOMMU
      +--> configure interrupts
      +--> initialize GPU
      |
      v
Device ready
```

Know:

- PCI configuration space
- BAR
- MMIO
- MSI/MSI-X
- DMA
- reset/recovery
- suspend/resume

---

## 6.11 GPU Command Submission — Complete Story

```text
OpenGL draw call
      |
      v
Mesa prepares GPU work
      |
      v
Resources / buffer objects
      |
      v
DRM ioctl
      |
      v
Kernel GPU driver
      |
      +--> validate
      +--> map resources
      +--> build/submit job
      +--> add dependencies
      |
      v
GPU scheduler
      |
      v
GPU engine
      |
      v
Execution
      |
      v
Completion
      |
      v
IRQ / completion handling
      |
      v
Fence signal
      |
      v
Dependent work / userspace progress
```

### Important interview point

The exact command format, engine names and scheduler implementation are hardware/driver dependent. Explain the architecture without pretending all vendors implement it identically.

---

## 6.12 GPU Hang Debugging — Staff-Level Method

Start with the symptom and narrow the layer.

```text
Application symptom
      |
      v
OpenGL/Mesa
      |
      v
DRM ioctl
      |
      v
Kernel GPU driver
      |
      +--> object lifetime
      +--> GPU VM
      +--> command submission
      +--> scheduler
      +--> synchronization
      +--> IRQ
      |
      v
Hardware / firmware
```

### Checklist

**Userspace**

- invalid resource/state
- incorrect API usage
- command generation
- buffer lifetime

**DRM/kernel**

- invalid handle/object
- ioctl validation
- driver state
- memory mapping
- scheduler state

**GPU memory**

- GPU VA mapping
- IOMMU fault
- DMA mapping
- stale/freed buffer
- access permission

**Execution**

- invalid command
- dependency deadlock
- scheduler stall
- engine fault

**Interrupt/completion**

- did the GPU signal completion?
- did the IRQ arrive?
- did the handler process it?
- did the fence signal?

**Hardware/firmware**

- GPU engine fault
- firmware issue
- hardware failure
- reset/recovery path

---

## 6.13 Graphics Debugging Toolkit

Know what each tool is for:

```text
dmesg                    -> kernel/driver messages
journalctl               -> system journal
ls /dev/dri/             -> DRM device nodes
lspci                    -> PCI device/resource information
cat /proc/interrupts     -> interrupt activity
/sys/class/drm/          -> DRM/KMS sysfs state
/sys/kernel/debug/dri/   -> driver debug information
strace                   -> userspace syscalls
gdb                      -> userspace debugging
perf                     -> performance/profiling
ftrace                   -> kernel tracing
trace-cmd                -> trace collection/analysis
```

Vendor-specific GPU debug facilities should be treated as an additional layer rather than assumed to be identical across AMD/NVIDIA/Intel.

---

# Chapter 7 — Interview Questions That Matter for Your Profile

## Linux Graphics Architecture

1. Explain OpenGL application → Mesa → libdrm → DRM → GPU driver.
2. Mesa vs DRM: what runs in userspace and what runs in the kernel?
3. What does libdrm provide?
4. Why does userspace use `ioctl()` to communicate with DRM?
5. What is a DRM device node?
6. Card node vs render node?

## DRM/KMS

7. What is DRM?
8. What is KMS?
9. Explain connector, encoder, CRTC and plane.
10. What is a framebuffer?
11. What is atomic modesetting?
12. Why is atomic state useful?
13. What happens during an atomic commit?

## GPU Memory

14. CPU VA vs physical address vs GPU VA vs DMA address?
15. Why is an IOMMU useful?
16. What is DMA-BUF?
17. What problem does PRIME solve?
18. GEM vs TTM?
19. How do you manage buffer lifetime while GPU work is outstanding?
20. Why are fences needed?

## Command Submission

21. What is a GPU command buffer?
22. What is a GPU engine/queue?
23. What does a GPU scheduler do?
24. Explain a command from Mesa to hardware.
25. What happens when a dependency is not satisfied?

## Interrupts / Failure

26. What can generate a GPU interrupt?
27. What is a GPU hang?
28. How do you debug a GPU hang?
29. How do you distinguish a userspace bug from a kernel driver bug?
30. How do you investigate an IOMMU fault?
31. What happens if completion IRQ is lost?
32. How would you reason about a fence that never signals?

## Linux Driver Cross-Questions

33. Can an IRQ handler sleep?
34. Mutex vs spinlock?
35. `GFP_KERNEL` vs `GFP_ATOMIC`?
36. `kmalloc()` vs `vmalloc()`?
37. What happens in PCI driver `probe()`?
38. Why use the Linux DMA API?
39. Why use `copy_from_user()`?
40. How would you debug a use-after-free in a GPU driver?

---

# Chapter 8 — Five Flows You Must Be Able to Draw on a Whiteboard

## Flow 1 — OpenGL to GPU

```text
Application
  ↓
OpenGL
  ↓
Mesa
  ↓
libdrm
  ↓
ioctl
  ↓
DRM
  ↓
GPU kernel driver
  ↓
command submission
  ↓
GPU
```

## Flow 2 — GPU to Display

```text
GPU rendering
  ↓
Framebuffer / buffer
  ↓
DRM/KMS
  ↓
Plane
  ↓
CRTC
  ↓
Encoder
  ↓
Connector
  ↓
Display
```

## Flow 3 — GPU Memory

```text
Buffer object
  ↓
GPU VA mapping
  ↓
GPU MMU / IOMMU as applicable
  ↓
VRAM / system memory
  ↓
DMA
  ↓
GPU
```

## Flow 4 — Shared Buffer

```text
Producer
  ↓
DMA-BUF
  ↓
Shared buffer
  ↓
Consumer
  ↓
Fence / synchronization
  ↓
Safe access
```

## Flow 5 — GPU Completion

```text
GPU executes job
  ↓
Completion
  ↓
IRQ
  ↓
Driver handler
  ↓
Update completion state
  ↓
Fence signal
  ↓
Wake dependent work
```

---

# Final Positioning for Your Interviews

Your strongest technical story is:

```text
C/C++
   +
Linux System Programming
   +
Linux Kernel Concepts
   +
OpenGL
   +
Mesa
   +
libdrm
   +
DRM/KMS
   +
GPU Driver Architecture
   +
DMA / IOMMU / PCIe
   +
Multithreading / Synchronization
   +
Debugging / Performance
```

For a 15+ year interview, do not stop at definitions. For every subsystem, be ready to answer:

```text
What is it?
Why does it exist?
Where does it run?
Who calls it?
What state/data does it manage?
What happens on the happy path?
What happens on failure?
How is it synchronized?
How would you debug it?
What did you personally work on?
```

That final question is especially important: distinguish **hands-on experience** from **interview knowledge**. Do not claim implementation ownership for kernel/GPU components you studied but did not personally develop.
