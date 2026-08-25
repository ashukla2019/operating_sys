# Linux Graphics Driver — Complete Architecture & Triangle Rendering

## 1. Complete Graphics Architecture

```text
                         USER SPACE
┌────────────────────────────────────────────────────────────┐
│                  OpenGL Application                         │
└────────────────────────────┬───────────────────────────────┘
                             │
                             │ OpenGL
                             ▼
┌────────────────────────────────────────────────────────────┐
│                         Mesa                               │
│              OpenGL implementation / driver               │
│                                                            │
│          Rendering / GPU command generation                │
└────────────────────────────┬───────────────────────────────┘
                             │
                             │ DRM APIs / libdrm
                             ▼
┌────────────────────────────────────────────────────────────┐
│                         libdrm                             │
│             User-space interface to DRM                   │
└────────────────────────────┬───────────────────────────────┘
                             │
                             │ ioctl()
                             ▼

                         KERNEL
┌────────────────────────────────────────────────────────────┐
│                          DRM                               │
│               Direct Rendering Manager                     │
│                                                            │
│   ┌────────────────────────┐  ┌────────────────────────┐   │
│   │       GPU side         │  │       KMS side         │   │
│   │                        │  │                        │   │
│   │ GEM / Buffer Objects   │  │ Framebuffers            │   │
│   │ DMA-BUF                │  │ Planes                  │   │
│   │ Command submission     │  │ CRTCs                   │   │
│   │ Synchronization        │  │ Encoders                │   │
│   │ GPU memory             │  │ Connectors              │   │
│   └────────────┬───────────┘  └────────────┬───────────┘   │
│                │                           │               │
└────────────────┼───────────────────────────┼───────────────┘
                 │                           │
                 ▼                           ▼
        GPU Kernel Driver             Display Hardware
                 │                    / Display Engine
                 ▼                           │
                GPU                          │
                                             ▼
                                      HDMI / DisplayPort
                                             │
                                             ▼
                                          Monitor

```

2. The Most Important Mental Model

The entire graphics stack can be divided into two major jobs:

Job 1 — Rendering
OpenGL Application
        ↓
      Mesa
        ↓
     libdrm
        ↓
       DRM
        ↓
GPU Kernel Driver
        ↓
       GPU
        ↓
   Framebuffer

This answers:

How do we create the pixels?

Job 2 — Display
Framebuffer
     ↓
   KMS
     ↓
Display Engine
     ↓
HDMI / DisplayPort
     ↓
Monitor

This answers:

How do we get those pixels onto the screen?

3. CPU vs GPU
CPU

CPU is optimized for:

General-purpose workloads
Low latency
Complex control flow
Branch-heavy workloads
Sequential/dependent operations
Operating-system work
Networking
Filesystem
Application logic

Conceptually:

CPU
┌──────────────────────────────────────┐
│ Core │ Core │ Core │ Core │ ...      │
│                                      │
│ Complex cores                        │
│ Large/sophisticated caches           │
│ Branch prediction                    │
│ Out-of-order execution               │
│ Speculation                          │
└──────────────────────────────────────┘

CPU has a relatively small number of powerful cores.

4. GPU

GPU is optimized for:

Massive parallelism
High throughput
Graphics rendering
Image processing
Matrix operations
Machine learning
Video processing
Data-parallel workloads

Conceptually:

GPU
┌──────────────────────────────────────────────┐
│ Compute/Shader │ Compute/Shader │ ...        │
│      Unit      │      Unit      │             │
│                │                │             │
│ many execution │ many execution │             │
│     lanes      │     lanes      │             │
└──────────────────────────────────────────────┘

A GPU has a very large number of execution resources compared with a CPU.

5. CPU vs GPU — Interview Comparison
CPU	GPU
Few powerful cores	Many parallel execution resources
Low latency	High throughput
General purpose	Parallel-workload oriented
Excellent control flow	Best with regular control flow
Excellent branch handling	Branch divergence can hurt performance
Complex cores	Many simpler/specialized execution resources
OS/application execution	Graphics/compute acceleration
Sequential workloads	Massively parallel workloads

Important:

Do not simply say "CPU is sequential and GPU is parallel."

CPUs also perform parallel execution using:

Multiple cores
SIMD
Superscalar execution
Out-of-order execution

Better answer:

CPU hardware is optimized for flexibility, low latency, and complex control flow, while GPU hardware is optimized for massive throughput and data-parallel execution.

6. Why GPUs Are Good for Graphics

Suppose we have a 1920 × 1080 image:

1920 × 1080 ≈ 2 million pixels

Suppose every pixel needs the same operation:

output_pixel = input_pixel × brightness

This is highly parallel:

Pixel 1 ──┐
Pixel 2 ──┤
Pixel 3 ──┤
Pixel 4 ──┼──> GPU
Pixel 5 ──┤
Pixel 6 ──┤
...       │
Pixel 2M ─┘

The GPU can process many independent elements concurrently.

7. GPU Branch Divergence

GPUs prefer similar execution paths.

For example:

if (x > 0)
    A();
else
    B();

Suppose GPU threads encounter:

Thread 1 → A
Thread 2 → A
Thread 3 → B
Thread 4 → B

The execution group may need to execute both paths.

                 Branch
                   |
          ┌────────┴────────┐
          ↓                 ↓
        A path            B path
          ↓                 ↓
       some threads       some threads

This is called:

Branch divergence

It can reduce GPU efficiency.

8. Component #1 — OpenGL Application

The application is the component that requests rendering.

For example:

float vertices[] = {
     0.0f,  0.8f, 0.0f,
    -0.8f, -0.8f, 0.0f,
     0.8f, -0.8f, 0.0f
};

This describes:

              V0
              /\
             /  \
            /    \
           /      \
        V1 -------- V2

Then:

glDrawArrays(GL_TRIANGLES, 0, 3);

The application is saying:

"OpenGL, please draw these three vertices as a triangle."

The application does not normally directly program GPU registers.

9. What Is OpenGL?

OpenGL is a graphics API.

Examples:

glDrawArrays(...)
glBindBuffer(...)
glUseProgram(...)
glClear(...)

These are API calls.

Conceptually:

Application
     |
     | OpenGL API
     ▼
OpenGL implementation

On Linux, Mesa provides much of the OpenGL implementation.

10. Component #2 — Mesa

Mesa is primarily a user-space graphics software stack.

For OpenGL:

Application
     |
     | OpenGL
     ▼
   Mesa

Mesa takes something like:

glDrawArrays(GL_TRIANGLES, 0, 3);

and determines what GPU state and commands are required.

11. What Does Mesa Do?

Conceptually:

OpenGL API
    ↓
Shader handling
    ↓
Resource management
    ↓
GPU state
    ↓
GPU command generation
    ↓
Command submission

For example, the application says:

glDrawArrays(GL_TRIANGLES, 0, 3);

Mesa may need to prepare:

Vertex buffer
Shader program
Render target
GPU state
Pipeline state
Draw command

Conceptually:

             Mesa
               |
       ┌───────┼────────┐
       ↓       ↓        ↓
   Vertex    Shader   Framebuffer
   Buffer    State      State
       \       |       /
        \      |      /
         ▼     ▼     ▼
          GPU Commands
12. Important — Mesa Does Not Normally Execute the Triangle

Mesa prepares work for the GPU.

Think:

Mesa
  |
  | "Here are the commands"
  ▼
GPU
  |
  | actually executes
  ▼
Triangle

Mesa is software running primarily in user space.

The GPU is the hardware that executes the graphics workload.

13. Component #3 — libdrm

Mesa needs a way to communicate with the Linux DRM subsystem.

This is where libdrm can be used.

Conceptually:

Mesa
  |
  ▼
libdrm
  |
  ▼
DRM kernel subsystem

libdrm provides user-space helper/library interfaces for interacting with DRM devices and APIs.

14. What Is ioctl()?

ioctl() is a system-call mechanism used to perform device-specific operations.

Conceptually:

User Space
────────────────

Mesa
  |
  ▼
libdrm
  |
  ▼
ioctl()
  |
  |
========== USER/KERNEL BOUNDARY ==========
  |
  ▼

Kernel
  |
  ▼
DRM

Think of ioctl() as:

"Kernel, perform this device-specific operation for me."

15. Component #4 — DRM

DRM means:

Direct Rendering Manager

DRM is a Linux kernel graphics framework.

It is NOT simply:

DRM = GPU

A better mental model is:

                  DRM
                   |
        ┌──────────┴──────────┐
        ↓                     ↓
   GPU management          KMS/display

DRM provides common infrastructure for graphics devices.

16. Why Do We Need DRM?

Without a kernel graphics framework, applications would potentially need to understand:

GPU registers
GPU memory
DMA
GPU scheduling
Synchronization
Interrupts
Display hardware

That would be unsafe and difficult.

Instead:

Application
     ↓
Mesa
     ↓
DRM
     ↓
GPU Driver
     ↓
Hardware

Linux provides common infrastructure through DRM.

17. DRM GPU Side

The GPU side of DRM includes concepts such as:

GEM / Buffer Objects
DMA-BUF
Command submission
Synchronization
GPU memory management
Scheduling

Let's understand each.

18. GEM / Buffer Objects

Graphics applications use many buffers:

Vertex Buffer
Texture
Framebuffer
Index Buffer
Command Buffer

The kernel needs to manage these resources.

A buffer object represents a kernel-managed graphics buffer/resource.

Conceptually:

Buffer Object
┌────────────────────────────┐
│                            │
│      Vertex Data           │
│                            │
└────────────────────────────┘

GEM is one of the DRM memory-management frameworks historically used for managing graphics buffer objects.

Important:

GEM is not simply "physical VRAM."

Better:

GEM provides kernel infrastructure for managing graphics memory objects.

19. DMA-BUF

Suppose a buffer needs to be shared between different devices/subsystems.

For example:

Camera
   |
   ▼
Buffer
   |
   ├──── GPU
   |
   └──── Display

DMA-BUF provides a mechanism for sharing DMA-capable buffers between devices/subsystems.

Conceptually:

            Shared Buffer
                 |
        ┌────────┼────────┐
        ↓        ↓        ↓
       GPU     Camera   Display

Important:

DMA-BUF is primarily about buffer sharing, not GPU rendering itself.

20. Command Submission

Mesa prepares GPU commands.

Conceptually:

Command Buffer
┌──────────────────────────┐
│ Set state                │
│ Bind vertex buffer       │
│ Bind shader              │
│ Bind framebuffer         │
│ Draw triangle            │
└──────────────────────────┘

The kernel GPU driver needs to submit this work to the GPU.

Mesa
 |
 ▼
DRM
 |
 ▼
GPU Kernel Driver
 |
 ▼
GPU Command Processor
21. Synchronization

Suppose:

GPU is rendering framebuffer A

while another component wants to display framebuffer A.

We cannot blindly allow both to access the resource at the wrong time.

We need synchronization.

Conceptually:

GPU rendering
     |
     | busy
     ▼
 Framebuffer
     |
     | wait
     ▼
 Display

Graphics systems use synchronization mechanisms such as:

Fence
Sync object
DMA-BUF fence
Timeline synchronization

Exact mechanisms depend on the graphics stack and driver.

22. GPU Memory

The GPU needs memory for:

Vertex data
Textures
Shaders/resources
Command buffers
Framebuffer

Conceptually:

GPU Memory
┌──────────────────────────────┐
│ Vertex Buffer                │
├──────────────────────────────┤
│ Index Buffer                 │
├──────────────────────────────┤
│ Texture                      │
├──────────────────────────────┤
│ Command Buffer               │
├──────────────────────────────┤
│ Framebuffer                  │
└──────────────────────────────┘

The kernel driver manages the resources and mappings needed to make these buffers accessible to the GPU.

23. Component #5 — GPU Kernel Driver

The GPU kernel driver is the hardware-specific kernel component.

Conceptually:

             DRM
              |
              ▼
      GPU Kernel Driver
              |
       ┌──────┼───────┐
       ↓      ↓       ↓
      MMIO    DMA    IRQ
              |
              ▼
             GPU

The exact driver depends on the hardware.

Examples:

Intel GPU → i915 / newer Xe driver stack
AMD GPU   → amdgpu
NVIDIA    → NVIDIA kernel driver / Nouveau
24. What Does the GPU Kernel Driver Do?

Typical responsibilities include:

GPU initialization
GPU memory management
Command submission
GPU scheduling
Interrupt handling
Synchronization
Power management
Hardware configuration
MMIO
DMA
Context management

This is the layer that knows how to communicate with the actual GPU hardware.

25. MMIO

MMIO means:

Memory-Mapped I/O

Hardware registers are exposed in an address space that the CPU can access.

Conceptually:

CPU
 |
 | read/write
 ▼
MMIO Register
 |
 ▼
GPU Hardware

Examples conceptually:

GPU_STATUS
GPU_CONTROL
GPU_INTERRUPT_STATUS

The driver accesses these registers to control and inspect hardware.

26. DMA

DMA means:

Direct Memory Access

DMA allows a device to transfer data to/from memory without requiring the CPU to perform every individual copy.

Conceptually:

             DMA
              |
      ┌───────┴────────┐
      ↓                ↓
    Memory            GPU

This is important for high-throughput graphics workloads.

27. GPU Interrupts

The GPU may need to notify the CPU:

"My work completed."

Conceptually:

GPU
 |
 | Interrupt
 ▼
CPU
 |
 ▼
GPU Driver

The driver handles the interrupt and updates state/synchronization.

Example conceptually:

static irqreturn_t gpu_irq(int irq, void *data)
{
    u32 status;

    status = ioread32(regs + GPU_STATUS_REG);

    if (status & GPU_JOB_COMPLETE) {
        /* Acknowledge interrupt */
        return IRQ_HANDLED;
    }

    return IRQ_NONE;
}
28. Component #6 — GPU

The GPU receives commands such as:

Bind vertex buffer
Bind shader
Configure pipeline
Draw triangle

The GPU then performs the actual graphics processing.

29. GPU Rendering Pipeline

A simplified graphics pipeline:

Vertex Data
    |
    ▼
Vertex Shader
    |
    ▼
Primitive Assembly
    |
    ▼
Rasterization
    |
    ▼
Fragment Shader
    |
    ▼
Depth / Stencil / Blend
    |
    ▼
Framebuffer
30. Vertex Shader

Input:

V0
V1
V2

The vertex shader transforms the vertices.

Conceptually:

Model
  ↓
World
  ↓
View
  ↓
Projection

Result:

Transformed V0
Transformed V1
Transformed V2
31. Primitive Assembly

The GPU knows:

GL_TRIANGLES

Therefore:

V0 + V1 + V2

forms one triangle.

       V0
       /\
      /  \
     /    \
    /      \
   V1------V2
32. Rasterization

The rasterizer converts geometry into fragments.

Triangle
   |
   ▼
Rasterizer
   |
   ▼
Fragments

Conceptually:

        /\
       /##\
      /####\
     /######\
    /########\

The rasterizer determines which screen locations are covered.

33. Fragment Shader

Each fragment is processed by the fragment shader.

Fragment
    |
    ▼
Fragment Shader
    |
    ▼
RGBA Color

Example:

R = 1.0
G = 0.0
B = 0.0
A = 1.0

So the triangle can become red.

34. Depth / Stencil / Blending

The GPU may perform additional tests and operations:

Fragment
   |
   ▼
Depth Test
   |
   ▼
Stencil Test
   |
   ▼
Blending
   |
   ▼
Framebuffer

These determine whether and how the fragment contributes to the final image.

35. Framebuffer

The final pixel values are written to a framebuffer.

Conceptually:

Framebuffer

┌─────────────────────────────┐
│                             │
│            RED              │
│           /###\             │
│          /#####\            │
│         /#######\           │
│        /#########\          │
│                             │
└─────────────────────────────┘

At this point:

The GPU has created the image.

But the monitor has not necessarily received it yet.

36. Component #7 — KMS

KMS means:

Kernel Mode Setting

KMS is the display-management portion of DRM.

Its job is to configure the display pipeline.

Conceptually:

GPU Rendering
      |
      ▼
 Framebuffer
      |
      ▼
     KMS
      |
      ▼
Display Hardware
      |
      ▼
   Monitor

KMS deals with things such as:

Resolution
Refresh rate
Display mode
Framebuffer
Planes
CRTCs
Encoders
Connectors
Display configuration
37. DRM vs KMS

Do not simply say:

DRM = GPU
KMS = Screen

That is too simplistic.

Better:

                    DRM
                     |
          ┌──────────┴──────────┐
          │                     │
          ▼                     ▼
    GPU infrastructure      KMS / Display
          │                     │
          ▼                     ▼
      GPU Driver          Display Pipeline

The correct interview statement is:

DRM is the Linux kernel graphics framework. KMS is the display-management part of DRM.

38. KMS Side

KMS contains concepts such as:

Framebuffer
Plane
CRTC
Encoder
Connector

Let's understand them.

39. KMS Framebuffer

A DRM framebuffer represents the configuration describing how pixel data in a buffer should be interpreted for display.

Conceptually:

Framebuffer
     |
     ▼
Pixel Buffer
     |
     ▼
Display Pipeline

Important:

A DRM framebuffer object is not necessarily identical to the underlying memory allocation.

It describes how a buffer/resource is used for display.

40. Plane

A plane is a display source.

For example:

Plane
  |
  ▼
Framebuffer

Modern display hardware may have multiple planes:

Primary Plane
Cursor Plane
Overlay Plane

Conceptually:

Background
     +
Video Overlay
     +
Cursor
     |
     ▼
Final Display

Display hardware may compose these without requiring the 3D engine to render everything into a single buffer.

41. CRTC

CRTC historically means:

Cathode Ray Tube Controller

Modern hardware does not necessarily use CRTs, but the DRM object is still called CRTC.

The CRTC is associated with:

Display timing
Resolution
Refresh rate
Scanout

Conceptually:

Framebuffer
     |
     ▼
   Plane
     |
     ▼
   CRTC
     |
     ▼
Pixel Stream
42. Encoder

The encoder handles the display output path.

Conceptually:

CRTC
  |
  ▼
Encoder
  |
  ▼
HDMI / DisplayPort
43. Connector

A connector represents a display output connection.

Examples:

HDMI
DisplayPort
DVI

Conceptually:

Encoder
   |
   ▼
Connector
   |
   ▼
Cable
   |
   ▼
Monitor
44. Display Engine

The display engine is hardware responsible for scanning out the framebuffer and producing the display stream.

Conceptually:

Framebuffer
     |
     ▼
Display Engine
     |
     ▼
Display Stream
     |
     ▼
HDMI / DisplayPort

Important:

The display engine is generally part of the graphics/display hardware associated with the GPU.

It is not necessarily a completely separate device.

45. Full Triangle Rendering Flow

Now connect everything.

Application
     |
     | glDrawArrays()
     ▼
   OpenGL
     |
     ▼
    Mesa
     |
     | Prepare GPU state + commands
     ▼
   libdrm
     |
     | ioctl()
     ▼
    DRM
     |
     ▼
GPU Kernel Driver
     |
     | Submit commands
     ▼
    GPU
     |
     ▼
Vertex Shader
     |
     ▼
Primitive Assembly
     |
     ▼
Rasterizer
     |
     ▼
Fragment Shader
     |
     ▼
Depth / Stencil / Blend
     |
     ▼
Framebuffer
     |
     ▼
KMS
     |
     ▼
Plane
     |
     ▼
CRTC
     |
     ▼
Encoder
     |
     ▼
Connector
     |
     ▼
HDMI / DisplayPort
     |
     ▼
Monitor
46. Rendering Path vs Display Path
Rendering Path
OpenGL Application
        |
        ▼
      Mesa
        |
        ▼
     libdrm
        |
        ▼
       DRM
        |
        ▼
GPU Kernel Driver
        |
        ▼
       GPU
        |
        ▼
   Framebuffer

Purpose:

Generate the image.

Display Path
Framebuffer
     |
     ▼
    KMS
     |
     ▼
   Plane
     |
     ▼
   CRTC
     |
     ▼
 Encoder
     |
     ▼
Connector
     |
     ▼
HDMI / DisplayPort
     |
     ▼
 Monitor

Purpose:

Display the image.

47. Complete Architecture With Both Paths
                         USER SPACE

┌────────────────────────────────────────────────────────────┐
│                  OpenGL Application                         │
└────────────────────────────┬───────────────────────────────┘
                             │
                             │ OpenGL
                             ▼
┌────────────────────────────────────────────────────────────┐
│                         Mesa                               │
│                                                            │
│        OpenGL implementation / user-space driver           │
│                                                            │
│        Shader handling                                     │
│        Resource management                                 │
│        GPU state                                           │
│        GPU command generation                              │
└────────────────────────────┬───────────────────────────────┘
                             │
                             │ DRM APIs
                             ▼
┌────────────────────────────────────────────────────────────┐
│                         libdrm                             │
└────────────────────────────┬───────────────────────────────┘
                             │
                             │ ioctl()
                             ▼

                         KERNEL

┌────────────────────────────────────────────────────────────┐
│                          DRM                               │
│                                                            │
│  ┌────────────────────────┐  ┌────────────────────────┐   │
│  │       GPU side         │  │       KMS side         │   │
│  │                        │  │                        │   │
│  │ Buffer Objects         │  │ Framebuffer            │   │
│  │ GEM                    │  │ Plane                  │   │
│  │ DMA-BUF                │  │ CRTC                   │   │
│  │ Command Submission     │  │ Encoder                │   │
│  │ Synchronization        │  │ Connector              │   │
│  │ GPU Memory             │  │ Display Configuration  │   │
│  └───────────┬────────────┘  └────────────┬───────────┘   │
└──────────────┼────────────────────────────┼───────────────┘
               │                            │
               ▼                            ▼
      GPU Kernel Driver             Display Hardware
               │                    / Display Engine
               ▼                            │
              GPU                           │
               │                            │
               └──────────┐                 │
                          ▼                 ▼
                     Framebuffer       HDMI / DisplayPort
                                            │
                                            ▼
                                         Monitor
48. Why User Space Cannot Directly Control Everything

Imagine two applications:

Application A
     |
     └── GPU

Application B
     |
     └── GPU

If both could freely modify GPU registers:

App A → GPU register
App B → same GPU register

they could interfere with each other.

Therefore:

User Space
     |
     | Controlled API / ioctl
     ▼
Kernel DRM
     |
     ▼
GPU Driver
     |
     ▼
Hardware

The kernel provides controlled access to the hardware.

49. User Space vs Kernel Space
                  USER SPACE
────────────────────────────────────────
Application
Mesa
libdrm
Compositor / Display Server


================ KERNEL BOUNDARY ==========


                  KERNEL SPACE
────────────────────────────────────────
DRM
GPU Kernel Driver
KMS
Hardware management


                  HARDWARE
────────────────────────────────────────
GPU
Display Engine
HDMI / DisplayPort
Monitor

This separation provides:

Security
Isolation
Resource management
Scheduling
Hardware protection
50. Where Does the Compositor Fit?

Modern Linux desktop systems usually have a compositor/display server.

For example:

Wayland compositor

Historically:

X server / X compositor

The compositor may interact with DRM/KMS to display the final desktop.

Conceptually:

Applications
    |
    ▼
OpenGL / Mesa
    |
    ▼
GPU
    |
    ▼
Rendered Buffers
    |
    ▼
Compositor
    |
    ▼
DRM / KMS
    |
    ▼
Display
51. Real Desktop Example

Suppose you have:

Browser
Terminal
Video Player

The compositor may have buffers representing:

┌────────────────────────────────────┐
│ Browser                            │
│                                    │
│        ┌──────────────┐            │
│        │ Terminal     │            │
│        │              │            │
│        └──────────────┘            │
│                        Video       │
└────────────────────────────────────┘

The final composition can be presented through DRM/KMS.

Modern display hardware may also use multiple planes to reduce GPU composition work.

52. Buffer Flow

One important concept is that rendering and display can use buffers.

Conceptually:

                Buffer
                  |
          ┌───────┴────────┐
          │                │
          ▼                ▼
        GPU             Display
          │                │
       writes            reads
          │                │
          └───────┬────────┘
                  │
             Synchronization

The GPU must finish rendering before display scans out the appropriate buffer.

This is where synchronization becomes important.

53. Double Buffering

A simplified double-buffering model:

Buffer A → currently displayed
Buffer B → GPU rendering

While the monitor scans out A:

Monitor
   ↑
   |
Buffer A

the GPU renders into B:

GPU
 |
 ▼
Buffer B

Then they can swap:

Before:

Display → A
GPU     → B


After:

Display → B
GPU     → A

This helps avoid displaying a partially rendered frame.

54. VBlank

Display hardware periodically reaches a vertical blanking interval.

Conceptually:

Frame N
   |
   ▼
Scanout
   |
   ▼
VBlank
   |
   ▼
Frame N+1

Page flips/display updates can be synchronized with display timing to avoid visual artifacts.

55. Page Flip

A simplified idea:

Old framebuffer
       |
       ▼
     Display


New framebuffer
       |
       ▼
     GPU rendering

At an appropriate point:

Display → New framebuffer

This is commonly referred to as a page flip in DRM/KMS terminology.

56. Important Distinction — Buffer vs Framebuffer

Do not say:

"A framebuffer is just VRAM."

More accurate:

Memory Buffer
      |
      ▼
DRM Framebuffer Object
      |
      ▼
KMS Display Pipeline

The underlying buffer contains pixel data.

The DRM framebuffer describes how that buffer is used for scanout.

57. Important Distinction — DRM vs GPU Driver

Do not say:

"DRM is the GPU driver."

Better:

DRM
 |
 └── Common kernel graphics framework
          |
          ▼
   Hardware-specific driver
          |
          ▼
         GPU

The driver uses DRM infrastructure and implements hardware-specific behavior.

58. Important Distinction — Mesa vs DRM

Mesa:

User Space

DRM:

Kernel Space

Conceptually:

USER SPACE
──────────────────
Application
     ↓
   Mesa
     ↓
 libdrm
──────────────────
     ↓
   ioctl()
──────────────────
KERNEL SPACE
     ↓
   DRM
     ↓
GPU Kernel Driver
59. Important Distinction — KMS vs Display Engine

KMS is software/kernel infrastructure.

Display Engine is hardware.

KMS
 |
 | configures
 ▼
Display Engine
 |
 | scans out
 ▼
HDMI / DisplayPort
 |
 ▼
Monitor

Therefore:

KMS is not the physical display engine.

60. Important Interview Question
"Is KMS part of DRM?"

Yes.

Conceptually:

DRM
 |
 ├── GPU-related infrastructure
 │
 └── KMS
      |
      ├── Plane
      ├── CRTC
      ├── Encoder
      └── Connector

KMS is the kernel mode-setting/display-management portion of the DRM subsystem.

61. Important Interview Question
"Does DRM only handle rendering?"

No.

DRM provides broader graphics infrastructure.

It supports:

GPU resource management
GPU command submission
Synchronization
Memory management
Display management
KMS
62. Important Interview Question
"Does KMS render the triangle?"

No.

The GPU rendering engine renders the triangle.

KMS configures how the resulting buffer is displayed.

GPU
 ↓
renders triangle
 ↓
Framebuffer
 ↓
KMS
 ↓
Display
63. Important Interview Question
"Does Mesa talk directly to GPU hardware?"

Conceptually, the normal protected architecture is:

Mesa
 ↓
DRM interface
 ↓
Kernel GPU driver
 ↓
GPU

Mesa prepares the GPU workload.

The kernel driver controls the hardware.

64. Important Interview Question
"Where is the actual triangle rendered?"

In the GPU hardware rendering pipeline:

Vertex Shader
      ↓
Primitive Assembly
      ↓
Rasterization
      ↓
Fragment Shader
      ↓
Framebuffer

Not inside:

Mesa
DRM
libdrm
KMS

Those layers prepare/manage/control the workload.

65. Important Interview Question
"Where is the triangle stored after rendering?"

Typically in a framebuffer/render target.

Conceptually:

GPU
 ↓
Render Target
 ↓
Framebuffer

That buffer can then be used for display scanout, potentially through KMS.

66. Important Interview Question
"Who sends pixels to the monitor?"

The GPU/display hardware's display engine performs scanout and produces the display stream.

Conceptually:

Framebuffer
     ↓
Display Engine
     ↓
HDMI / DisplayPort
     ↓
Monitor

KMS configures that hardware pipeline.

67. Full End-to-End Triangle Example

Suppose the application executes:

glDrawArrays(GL_TRIANGLES, 0, 3);
Step 1

Application requests a triangle.

Application
     ↓
glDrawArrays()
Step 2

Mesa receives the OpenGL operation.

glDrawArrays()
     ↓
Mesa
Step 3

Mesa prepares:

Vertex buffer
Shader
Pipeline state
Framebuffer
Draw command
Step 4

The user-space stack communicates with DRM.

Mesa
 ↓
libdrm
 ↓
ioctl()
Step 5

Kernel DRM processes the request.

ioctl()
 ↓
DRM
Step 6

The hardware-specific GPU driver manages the GPU submission.

DRM
 ↓
GPU Kernel Driver
 ↓
GPU
Step 7

GPU executes:

Vertex Shader
 ↓
Primitive Assembly
 ↓
Rasterizer
 ↓
Fragment Shader
 ↓
Depth/Stencil/Blend
Step 8

Pixels are written:

GPU
 ↓
Framebuffer
Step 9

Display pipeline is configured:

Framebuffer
 ↓
KMS
 ↓
Plane
 ↓
CRTC
 ↓
Encoder
 ↓
Connector
Step 10

Display hardware sends the stream:

Connector
 ↓
HDMI / DisplayPort
 ↓
Monitor
68. Complete Triangle Flow — One Diagram
                         USER SPACE
┌──────────────────────────────────────────────────────┐
│                OpenGL Application                    │
│                                                      │
│          glDrawArrays(GL_TRIANGLES...)               │
└─────────────────────────┬────────────────────────────┘
                          │
                          ▼
┌──────────────────────────────────────────────────────┐
│                        Mesa                          │
│                                                      │
│  OpenGL implementation                              │
│  Shader handling                                    │
│  Resource management                                │
│  GPU state                                          │
│  Command generation                                 │
└─────────────────────────┬────────────────────────────┘
                          │
                          ▼
┌──────────────────────────────────────────────────────┐
│                       libdrm                         │
└─────────────────────────┬────────────────────────────┘
                          │
                        ioctl()
                          │
══════════════════════════╪══════════════════════════════
                          │
                         KERNEL
                          ▼
┌──────────────────────────────────────────────────────┐
│                        DRM                           │
│                                                      │
│  Buffer management                                  │
│  Command submission                                 │
│  Synchronization                                    │
│  GPU memory management                              │
│  KMS / Display management                           │
└─────────────────────────┬────────────────────────────┘
                          │
                          ▼
                 GPU Kernel Driver
                          │
                          ▼
                         GPU
                          │
             ┌────────────┴────────────┐
             │                         │
             ▼                         │
       Vertex Shader                   │
             │                         │
             ▼                         │
      Primitive Assembly               │
             │                         │
             ▼                         │
         Rasterizer                   │
             │                         │
             ▼                         │
       Fragment Shader                 │
             │                         │
             ▼                         │
      Depth/Stencil/Blend             │
             │                         │
             ▼                         │
        Framebuffer ◄─────────────────┘
             │
             ▼
            KMS
             │
             ▼
           Plane
             │
             ▼
           CRTC
             │
             ▼
          Encoder
             │
             ▼
         Connector
             │
             ▼
       HDMI / DisplayPort
             │
             ▼
          Monitor
69. The Two Pipelines in One Diagram
                         USER SPACE

              OpenGL Application
                       │
                       ▼
                     Mesa
                       │
                       ▼
                    libdrm
                       │
                       ▼

                         KERNEL

                      DRM
                  /          \
                 /            \
                ▼              ▼
        GPU / Rendering       KMS / Display
                │              │
                ▼              ▼
       GPU Kernel Driver     Plane
                │              │
                ▼              ▼
               GPU           CRTC
                │              │
                ▼              ▼
          Render Pipeline   Encoder
                │              │
                ▼              ▼
           Framebuffer      Connector
                │              │
                └──────┬───────┘
                       │
                       ▼
                Display Engine
                       │
                       ▼
                HDMI / DisplayPort
                       │
                       ▼
                    Monitor
70. Component Summary
Component	Space	Main Role
OpenGL Application	User	Requests graphics operations
OpenGL	User/API	Graphics programming interface
Mesa	User	Implements OpenGL and prepares GPU work
libdrm	User	Interface/helpers for DRM
ioctl()	Boundary	Device-specific user/kernel operation
DRM	Kernel	Linux graphics framework
GEM / BO	Kernel	Graphics buffer/resource management
DMA-BUF	Kernel	Sharing buffers between devices/subsystems
GPU Scheduling	Kernel/Driver	Manages GPU execution
Synchronization	Kernel/Driver	Coordinates GPU/CPU/display work
GPU Kernel Driver	Kernel	Hardware-specific GPU control
MMIO	Kernel/Hardware	Access GPU registers
DMA	Hardware/Driver	Efficient memory transfers
IRQ	Hardware/Kernel	GPU event notification
GPU	Hardware	Executes graphics/compute work
Vertex Shader	GPU	Processes vertices
Rasterizer	GPU	Converts primitives to fragments
Fragment Shader	GPU	Calculates fragment output
Framebuffer	Memory/DRM	Stores pixel data
KMS	Kernel	Configures display pipeline
Plane	Display hardware	Provides framebuffer/display source
CRTC	Display hardware	Controls timing and scanout
Encoder	Display hardware	Handles output encoding
Connector	DRM/KMS	Represents display connection
Display Engine	Hardware	Scans out framebuffer
HDMI/DP	Hardware interface	Transfers display stream
Monitor	Hardware	Displays pixels
71. Interview Definitions
Mesa

Mesa is a user-space graphics stack that provides implementations such as OpenGL and translates graphics API operations into GPU-specific work.

libdrm

libdrm is a user-space library that provides interfaces/helpers for communicating with Linux DRM devices and APIs.

DRM

DRM, or Direct Rendering Manager, is the Linux kernel graphics framework that provides infrastructure for GPU resource management, command submission, synchronization, and display management.

KMS

KMS, or Kernel Mode Setting, is the display-management part of DRM used to configure display modes and the scanout pipeline.

GPU Kernel Driver

The GPU kernel driver is the hardware-specific kernel component that manages the GPU, including memory, command submission, synchronization, interrupts, scheduling, and hardware control.

Framebuffer

A framebuffer is a pixel buffer/resource used as a render target and/or display source. A DRM framebuffer object describes how a buffer is used for display.

Plane

A KMS plane is a display source that can reference a framebuffer and may be composed with other planes.

CRTC

A CRTC is a DRM/KMS display pipeline object associated with display timing and scanout.

Encoder

An encoder represents hardware that prepares the display stream for an output interface.

Connector

A connector represents a display output connection such as HDMI or DisplayPort.

Display Engine

The display engine is hardware that reads framebuffer data and produces the display stream for the physical output.

72. Common Interview Mistakes
Mistake 1

Wrong:

DRM = GPU driver.

Better:

DRM is the kernel graphics framework; the hardware-specific GPU driver operates within that framework.

Mistake 2

Wrong:

KMS is the monitor.

Better:

KMS is kernel-side display configuration/management.

Mistake 3

Wrong:

Mesa renders the triangle.

Better:

Mesa prepares the graphics workload; the GPU hardware executes the rendering pipeline.

Mistake 4

Wrong:

libdrm is the kernel driver.

Better:

libdrm is a user-space library/interface for interacting with DRM.

Mistake 5

Wrong:

Framebuffer = VRAM.

Better:

A framebuffer represents a displayable pixel resource; the underlying storage can have different memory implementations.

Mistake 6

Wrong:

KMS renders the image.

Better:

KMS configures the display pipeline and scanout; the GPU rendering engine generates the image.

73. Important Architecture Correction

Avoid drawing the architecture like this:

Mesa
  |
  ├── GPU work
  |
  └── Display work
          |
          ▼
        libdrm

This can incorrectly imply that Mesa is always responsible for both GPU rendering and display modesetting.

A better mental model is:

                  DRM
                   |
          ┌────────┴────────┐
          │                 │
          ▼                 ▼
    GPU infrastructure   KMS / Display
          │                 │
          ▼                 ▼
     GPU Driver       Display Pipeline

Also remember that display servers/compositors can interact with DRM/KMS.

74. Where the Compositor Fits

A modern Linux desktop can look conceptually like:

Application A ──┐
                │
Application B ──┼──> Compositor ──> DRM/KMS ──> Display
                │
Application C ──┘

Applications may render their individual surfaces into buffers.

The compositor decides how those surfaces are combined/presented.

Conceptually:

Browser Buffer
       +
Terminal Buffer
       +
Video Buffer
       +
Cursor
       |
       ▼
   Final Desktop
       |
       ▼
    DRM / KMS
       |
       ▼
    Display
75. Buffer Sharing With DMA-BUF

A common high-level flow can be:

Application
    |
    ▼
GPU renders
    |
    ▼
Graphics Buffer
    |
    | DMA-BUF sharing
    ▼
Compositor / Display
    |
    ▼
KMS

DMA-BUF allows buffers to be shared between different device/subsystem users without necessarily copying the entire image.

76. Double Buffering

A simplified model:

Buffer A → Display
Buffer B → GPU rendering

While:

Monitor
   ↑
   |
Buffer A

the GPU renders:

GPU
 |
 ▼
Buffer B

Then they swap:

Before:

Display → A
GPU     → B


After:

Display → B
GPU     → A

This helps prevent displaying a partially rendered frame.

77. VBlank

Display hardware periodically reaches a vertical blanking interval.

Conceptually:

Frame N
   |
   ▼
Scanout
   |
   ▼
VBlank
   |
   ▼
Frame N+1

Display updates can be synchronized with display timing.

78. Page Flip

A simplified page-flip concept:

Old framebuffer
       |
       ▼
     Display


New framebuffer
       |
       ▼
     GPU rendering

At an appropriate point:

Display → New framebuffer

This changes which framebuffer is scanned out.

79. Why KMS Matters

KMS allows the kernel/display subsystem to configure things such as:

Resolution
Refresh rate
Display mode
Framebuffer
Plane configuration
CRTC configuration
Connector
Display output

For example:

1920 × 1080 @ 60 Hz

or:

3840 × 2160 @ 60 Hz

The display hardware needs to know how to generate the appropriate scanout timing.

80. Final End-to-End Mental Model

Remember the following:

              WHAT DO I WANT TO DRAW?
                         |
                         ▼
                 OpenGL Application
                         |
                         ▼
                       Mesa
                         |
                 "Prepare GPU work"
                         |
                         ▼
                      libdrm
                         |
                       ioctl()
                         |
                         ▼
                       DRM
                    /        \
                   /          \
                  ▼            ▼
            GPU side         KMS side
               │                │
               ▼                ▼
        GPU Kernel Driver     Display Config
               │                │
               ▼                ▼
              GPU          Plane / CRTC
               │                │
               ▼                ▼
       Vertex/Rasterize       Encoder
               │                │
               ▼                ▼
          Framebuffer        Connector
               │                │
               └──────┬─────────┘
                      │
                      ▼
               Display Engine
                      │
                      ▼
              HDMI / DisplayPort
                      │
                      ▼
                   Monitor
81. One-Line Mental Model

Mesa prepares the GPU work, DRM provides the kernel graphics framework, the GPU driver controls the hardware, the GPU creates the pixels, and KMS/display hardware gets those pixels onto the screen.

82. 30-Second Interview Revision
Application
     ↓
OpenGL
     ↓
Mesa
     ↓
libdrm
     ↓
ioctl()
     ↓
DRM
     ↓
GPU Kernel Driver
     ↓
GPU
     ↓
Vertex Shader
     ↓
Primitive Assembly
     ↓
Rasterization
     ↓
Fragment Shader
     ↓
Depth/Stencil/Blend
     ↓
Framebuffer
     ↓
KMS
     ↓
Plane
     ↓
CRTC
     ↓
Encoder
     ↓
Connector
     ↓
HDMI/DP
     ↓
Monitor

Remember:

Mesa
→ Implements graphics API / prepares GPU work

DRM
→ Linux kernel graphics framework

GPU Driver
→ Hardware-specific GPU management

GPU
→ Actually renders the graphics

KMS
→ Configures display pipeline

Display Engine
→ Scans out framebuffer

Monitor
→ Displays the pixels
