# Linux Graphics Driver — Complete Architecture & Triangle Rendering

## 1. Complete Graphics Architecture

```
                         USER SPACE
┌────────────────────────────────────────────────────────────┐
│                  OpenGL Application                         │
└────────────────────────────┬───────────────────────────────┘
                              │ OpenGL
                              ▼
┌────────────────────────────────────────────────────────────┐
│                            Mesa                              │
│              OpenGL implementation / driver                  │
│              Rendering / GPU command generation               │
└────────────────────────────┬───────────────────────────────┘
                              │ DRM APIs / libdrm
                              ▼
┌────────────────────────────────────────────────────────────┐
│                          libdrm                              │
│               User-space interface to DRM                    │
└────────────────────────────┬───────────────────────────────┘
                              │ ioctl()
                              ▼
                          KERNEL
┌────────────────────────────────────────────────────────────┐
│                            DRM                                │
│                Direct Rendering Manager                       │
│                                                                │
│   ┌────────────────────────┐   ┌────────────────────────┐    │
│   │        GPU side         │   │        KMS side          │    │
│   │  GEM / Buffer Objects   │   │  Framebuffers             │    │
│   │  DMA-BUF                │   │  Planes                   │    │
│   │  Command submission     │   │  CRTCs                    │    │
│   │  Synchronization        │   │  Encoders                 │    │
│   │  GPU memory             │   │  Connectors                │    │
│   └────────────┬────────────┘   └────────────┬────────────┘    │
└────────────────┼────────────────────────────┼──────────────────┘
                  ▼                              ▼
          GPU Kernel Driver              Display Hardware
                  │                       / Display Engine
                  ▼                              │
                 GPU                             ▼
                                          HDMI / DisplayPort
                                                  │
                                                  ▼
                                               Monitor
```

---

## 2. The Most Important Mental Model

The entire graphics stack can be divided into two major jobs.

### Job 1 — Rendering ("How do we create the pixels?")

```
OpenGL Application → Mesa → libdrm → DRM → GPU Kernel Driver → GPU → Framebuffer
```

### Job 2 — Display ("How do we get those pixels onto the screen?")

```
Framebuffer → KMS → Display Engine → HDMI / DisplayPort → Monitor
```

---

## 3. CPU vs GPU

### CPU

CPU is optimized for:
- General-purpose workloads
- Low latency
- Complex control flow
- Branch-heavy workloads
- Sequential/dependent operations
- Operating-system work, networking, filesystem, application logic

```
CPU
┌──────────────────────────────────────┐
│ Core │ Core │ Core │ Core │ ...       │
│                                       │
│ Complex cores                        │
│ Large/sophisticated caches           │
│ Branch prediction                    │
│ Out-of-order execution               │
│ Speculation                          │
└──────────────────────────────────────┘
```

CPU has a relatively small number of powerful cores.

### GPU

GPU is optimized for:
- Massive parallelism
- High throughput
- Graphics rendering
- Image processing
- Matrix operations
- Machine learning
- Video processing
- Data-parallel workloads

```
GPU
┌────────────────────────────────────────────────┐
│ Compute/Shader │ Compute/Shader │ ...            │
│      Unit       │      Unit       │              │
│  many execution │  many execution │              │
│      lanes       │      lanes       │             │
└────────────────────────────────────────────────┘
```

A GPU has a very large number of execution resources compared with a CPU.

### CPU vs GPU — Interview Comparison

| CPU | GPU |
|---|---|
| Few powerful cores | Many parallel execution resources |
| Low latency | High throughput |
| General purpose | Parallel-workload oriented |
| Excellent control flow | Best with regular control flow |
| Excellent branch handling | Branch divergence can hurt performance |
| Complex cores | Many simpler/specialized execution resources |
| OS/application execution | Graphics/compute acceleration |
| Sequential workloads | Massively parallel workloads |

> **Important:** Don't simply say "CPU is sequential and GPU is parallel." CPUs also perform parallel execution via multiple cores, SIMD, superscalar execution, and out-of-order execution.
>
> **Better answer:** CPU hardware is optimized for flexibility, low latency, and complex control flow, while GPU hardware is optimized for massive throughput and data-parallel execution.

### Why GPUs Are Good for Graphics

For a 1920×1080 image (≈2 million pixels), if every pixel needs the same operation (`output_pixel = input_pixel × brightness`), this is highly parallel — the GPU can process many independent elements concurrently.

### GPU Branch Divergence

GPUs prefer similar execution paths across threads. For:

```c
if (x > 0)
    A();
else
    B();
```

If some threads take path A and others take path B, the execution group may need to execute *both* paths — some threads idle while others run. This is called **branch divergence**, and it can reduce GPU efficiency.

---

## 4. Component #1 — OpenGL Application

The application is the component that requests rendering.

```c
float vertices[] = {
     0.0f,  0.8f, 0.0f,
    -0.8f, -0.8f, 0.0f,
     0.8f, -0.8f, 0.0f
};

glDrawArrays(GL_TRIANGLES, 0, 3);
```

This describes a triangle (`V0`, `V1`, `V2`) and asks OpenGL to draw it. The application does **not** normally directly program GPU registers.

### What Is OpenGL?

OpenGL is a graphics **API** — e.g. `glDrawArrays()`, `glBindBuffer()`, `glUseProgram()`, `glClear()`. On Linux, **Mesa** provides much of the OpenGL implementation.

---

## 5. Component #2 — Mesa

Mesa is primarily a **user-space** graphics software stack. It takes a call like `glDrawArrays(GL_TRIANGLES, 0, 3)` and determines what GPU state and commands are required.

### What Does Mesa Do?

```
OpenGL API → Shader handling → Resource management → GPU state
           → GPU command generation → Command submission
```

Mesa prepares:
- Vertex buffer
- Shader program
- Render target
- GPU state / pipeline state
- Draw command

> **Important:** Mesa does *not* normally execute the triangle. Mesa prepares work for the GPU; the GPU is the hardware that actually executes the graphics workload.

---

## 6. Component #3 — libdrm

Mesa needs a way to communicate with the Linux DRM subsystem — that's **libdrm**.

```
Mesa → libdrm → DRM kernel subsystem
```

libdrm provides user-space helper/library interfaces for interacting with DRM devices and APIs.

### What Is `ioctl()`?

`ioctl()` is a system-call mechanism used to perform device-specific operations — think of it as: *"Kernel, perform this device-specific operation for me."*

```
User Space:  Mesa → libdrm → ioctl()
             ══════ USER/KERNEL BOUNDARY ══════
Kernel:      → DRM
```

---

## 7. Component #4 — DRM

**DRM = Direct Rendering Manager**, a Linux kernel graphics framework.

It is *not* simply "DRM = GPU." Better model:

```
                  DRM
                   │
        ┌──────────┴──────────┐
        ▼                     ▼
  GPU management          KMS/display
```

### Why Do We Need DRM?

Without a kernel graphics framework, applications would need to directly understand GPU registers, GPU memory, DMA, scheduling, synchronization, interrupts, and display hardware — unsafe and impractical. Instead:

```
Application → Mesa → DRM → GPU Driver → Hardware
```

### DRM — GPU Side

Concepts include: GEM/Buffer Objects, DMA-BUF, command submission, synchronization, GPU memory management, scheduling.

#### GEM / Buffer Objects

Graphics apps use many buffers (vertex, texture, framebuffer, index, command). A **buffer object** represents a kernel-managed graphics buffer/resource. GEM is one of the DRM memory-management frameworks historically used to manage these.

> GEM is **not** simply "physical VRAM" — it provides kernel infrastructure for managing graphics memory objects.

#### DMA-BUF

A mechanism for sharing DMA-capable buffers between devices/subsystems — e.g. a camera buffer shared with both the GPU and the display. DMA-BUF is about **buffer sharing**, not rendering itself.

#### Command Submission

Mesa prepares a command buffer (set state, bind vertex buffer, bind shader, bind framebuffer, draw triangle). The kernel GPU driver submits this work to the GPU:

```
Mesa → DRM → GPU Kernel Driver → GPU Command Processor
```

#### Synchronization

If the GPU is rendering framebuffer A while another component wants to display it, both can't access it carelessly at the wrong time — synchronization mechanisms are needed: fences, sync objects, DMA-BUF fences, timeline synchronization.

#### GPU Memory

The kernel driver manages GPU memory used for vertex data, textures, shaders/resources, command buffers, and the framebuffer.

---

## 8. Component #5 — GPU Kernel Driver

The hardware-specific kernel component.

```
DRM → GPU Kernel Driver → (MMIO / DMA / IRQ) → GPU
```

Examples: Intel (`i915` / Xe driver stack), AMD (`amdgpu`), NVIDIA (NVIDIA kernel driver / Nouveau).

### Responsibilities

GPU initialization, memory management, command submission, scheduling, interrupt handling, synchronization, power management, hardware configuration, MMIO, DMA, context management.

### MMIO (Memory-Mapped I/O)

Hardware registers exposed in an address space the CPU can access (e.g. `GPU_STATUS`, `GPU_CONTROL`, `GPU_INTERRUPT_STATUS`).

### DMA (Direct Memory Access)

Allows a device to transfer data to/from memory without the CPU copying every byte — important for high-throughput graphics.

### GPU Interrupts

The GPU notifies the CPU when work completes:

```c
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
```

---

## 9. Component #6 — GPU

The GPU receives commands (bind vertex buffer, bind shader, configure pipeline, draw triangle) and performs the actual graphics processing.

### GPU Rendering Pipeline

```
Vertex Data → Vertex Shader → Primitive Assembly → Rasterization
            → Fragment Shader → Depth/Stencil/Blend → Framebuffer
```

**Vertex Shader** — transforms vertices (Model → World → View → Projection).

**Primitive Assembly** — e.g. `GL_TRIANGLES` means V0+V1+V2 form one triangle.

**Rasterization** — converts geometry into fragments (which screen locations are covered).

**Fragment Shader** — computes an RGBA color per fragment (e.g. R=1.0, G=0.0, B=0.0, A=1.0 → red triangle).

**Depth / Stencil / Blending** — additional tests determining whether/how a fragment contributes to the final image.

**Framebuffer** — final pixel values are written here. At this point the GPU has created the image, but the monitor hasn't received it yet.

---

## 10. Component #7 — KMS (Kernel Mode Setting)

KMS is the **display-management** portion of DRM. Its job is to configure the display pipeline.

```
GPU Rendering → Framebuffer → KMS → Display Hardware → Monitor
```

KMS deals with: resolution, refresh rate, display mode, framebuffer, planes, CRTCs, encoders, connectors, display configuration.

### DRM vs KMS

Don't say "DRM = GPU, KMS = Screen." Better:

```
                    DRM
                     │
          ┌──────────┴──────────┐
          ▼                     ▼
    GPU infrastructure      KMS / Display
          │                     │
          ▼                     ▼
      GPU Driver          Display Pipeline
```

> **Correct interview statement:** DRM is the Linux kernel graphics framework. KMS is the display-management part of DRM.

### KMS Concepts

**Framebuffer** — describes how pixel data in a buffer should be interpreted for display. Not necessarily identical to the underlying memory allocation.

**Plane** — a display source referencing a framebuffer. Modern hardware may have a primary plane, cursor plane, and overlay plane(s), composed without the 3D engine rendering everything into one buffer.

**CRTC** (historically "Cathode Ray Tube Controller") — associated with display timing, resolution, refresh rate, and scanout.

**Encoder** — handles the display output path (CRTC → Encoder → HDMI/DisplayPort).

**Connector** — represents a display output connection (HDMI, DisplayPort, DVI).

**Display Engine** — hardware responsible for scanning out the framebuffer and producing the display stream. Generally part of the graphics/display hardware, not a fully separate device.

---

## 11. Full Triangle Rendering Flow

```
Application
     │  glDrawArrays()
     ▼
   OpenGL
     ▼
    Mesa            (prepares GPU state + commands)
     ▼
   libdrm
     │  ioctl()
     ▼
    DRM
     ▼
GPU Kernel Driver    (submits commands)
     ▼
    GPU
     ▼
Vertex Shader
     ▼
Primitive Assembly
     ▼
Rasterizer
     ▼
Fragment Shader
     ▼
Depth / Stencil / Blend
     ▼
Framebuffer
     ▼
    KMS
     ▼
   Plane
     ▼
   CRTC
     ▼
  Encoder
     ▼
 Connector
     ▼
HDMI / DisplayPort
     ▼
  Monitor
```

### Rendering Path vs Display Path

**Rendering Path** (generate the image):
```
OpenGL Application → Mesa → libdrm → DRM → GPU Kernel Driver → GPU → Framebuffer
```

**Display Path** (display the image):
```
Framebuffer → KMS → Plane → CRTC → Encoder → Connector → HDMI/DP → Monitor
```

---

## 12. Why User Space Cannot Directly Control Everything

If two applications could freely modify GPU registers directly, they could interfere with each other. So access is mediated:

```
User Space → Controlled API / ioctl → Kernel DRM → GPU Driver → Hardware
```

### User Space vs Kernel Space

| Layer | Components |
|---|---|
| **User Space** | Application, Mesa, libdrm, Compositor/Display Server |
| **Kernel Space** | DRM, GPU Kernel Driver, KMS, Hardware management |
| **Hardware** | GPU, Display Engine, HDMI/DisplayPort, Monitor |

This separation provides security, isolation, resource management, scheduling, and hardware protection.

---

## 13. Where Does the Compositor Fit?

Modern Linux desktops have a compositor/display server (Wayland compositor, or historically X server/X compositor), which interacts with DRM/KMS to display the final desktop.

```
Applications → OpenGL/Mesa → GPU → Rendered Buffers → Compositor → DRM/KMS → Display
```

**Example:** Browser + Terminal + Video Player windows are individually rendered, then the compositor combines their buffers into a final composed desktop image, presented through DRM/KMS. Modern display hardware may use multiple planes to reduce GPU composition work.

---

## 14. Buffer Flow & Synchronization

```
                Buffer
                  │
          ┌───────┴────────┐
          ▼                ▼
        GPU             Display
      (writes)          (reads)
          └───────┬────────┘
                  ▼
           Synchronization
```

The GPU must finish rendering before display scans out the buffer.

### Double Buffering

- Buffer A → currently displayed
- Buffer B → GPU is rendering into

Once rendering is done, they swap: Display → B, GPU → A. This avoids displaying a partially rendered frame.

### VBlank

Display hardware periodically reaches a **vertical blanking interval** between frames. Page flips/display updates are synchronized with this timing to avoid visual artifacts.

### Page Flip

At an appropriate point (typically during VBlank), the display switches from scanning out the old framebuffer to the new one — this is a **page flip**.

---

## 15. Key Distinctions (Common Interview Traps)

| ❌ Wrong | ✅ Better |
|---|---|
| DRM = GPU driver | DRM is the kernel graphics framework; the hardware-specific GPU driver operates within it |
| KMS is the monitor | KMS is kernel-side display configuration/management |
| Mesa renders the triangle | Mesa prepares the graphics workload; the GPU hardware executes the rendering pipeline |
| libdrm is the kernel driver | libdrm is a user-space library/interface for interacting with DRM |
| Framebuffer = VRAM | A framebuffer represents a displayable pixel resource; underlying storage can vary |
| KMS renders the image | KMS configures the display pipeline/scanout; the GPU generates the image |

---

## 16. Component Summary Table

| Component | Space | Main Role |
|---|---|---|
| OpenGL Application | User | Requests graphics operations |
| OpenGL | User/API | Graphics programming interface |
| Mesa | User | Implements OpenGL and prepares GPU work |
| libdrm | User | Interface/helpers for DRM |
| `ioctl()` | Boundary | Device-specific user/kernel operation |
| DRM | Kernel | Linux graphics framework |
| GEM / BO | Kernel | Graphics buffer/resource management |
| DMA-BUF | Kernel | Sharing buffers between devices/subsystems |
| GPU Scheduling | Kernel/Driver | Manages GPU execution |
| Synchronization | Kernel/Driver | Coordinates GPU/CPU/display work |
| GPU Kernel Driver | Kernel | Hardware-specific GPU control |
| MMIO | Kernel/Hardware | Access GPU registers |
| DMA | Hardware/Driver | Efficient memory transfers |
| IRQ | Hardware/Kernel | GPU event notification |
| GPU | Hardware | Executes graphics/compute work |
| Vertex Shader | GPU | Processes vertices |
| Rasterizer | GPU | Converts primitives to fragments |
| Fragment Shader | GPU | Calculates fragment output |
| Framebuffer | Memory/DRM | Stores pixel data |
| KMS | Kernel | Configures display pipeline |
| Plane | Display hardware | Provides framebuffer/display source |
| CRTC | Display hardware | Controls timing and scanout |
| Encoder | Display hardware | Handles output encoding |
| Connector | DRM/KMS | Represents display connection |
| Display Engine | Hardware | Scans out framebuffer |
| HDMI/DP | Hardware interface | Transfers display stream |
| Monitor | Hardware | Displays pixels |

---

## 17. Interview Definitions (Quick Reference)

- **Mesa** — A user-space graphics stack that provides implementations such as OpenGL and translates graphics API operations into GPU-specific work.
- **libdrm** — A user-space library that provides interfaces/helpers for communicating with Linux DRM devices and APIs.
- **DRM** — The Linux kernel graphics framework that provides infrastructure for GPU resource management, command submission, synchronization, and display management.
- **KMS** — The display-management part of DRM used to configure display modes and the scanout pipeline.
- **GPU Kernel Driver** — The hardware-specific kernel component that manages the GPU: memory, command submission, synchronization, interrupts, scheduling, hardware control.
- **Framebuffer** — A pixel buffer/resource used as a render target and/or display source. A DRM framebuffer object describes how a buffer is used for display.
- **Plane** — A KMS display source that can reference a framebuffer and may be composed with other planes.
- **CRTC** — A DRM/KMS display pipeline object associated with display timing and scanout.
- **Encoder** — Represents hardware that prepares the display stream for an output interface.
- **Connector** — Represents a display output connection such as HDMI or DisplayPort.
- **Display Engine** — Hardware that reads framebuffer data and produces the display stream for the physical output.

---

## 18. One-Line Mental Model

> Mesa prepares the GPU work, DRM provides the kernel graphics framework, the GPU driver controls the hardware, the GPU creates the pixels, and KMS/display hardware gets those pixels onto the screen.

---

## 19. 30-Second Interview Revision

```
Application → OpenGL → Mesa → libdrm → ioctl() → DRM → GPU Kernel Driver → GPU
   → Vertex Shader → Primitive Assembly → Rasterization → Fragment Shader
   → Depth/Stencil/Blend → Framebuffer → KMS → Plane → CRTC → Encoder
   → Connector → HDMI/DP → Monitor
```

| Layer | Role |
|---|---|
| **Mesa** | Implements graphics API / prepares GPU work |
| **DRM** | Linux kernel graphics framework |
| **GPU Driver** | Hardware-specific GPU management |
| **GPU** | Actually renders the graphics |
| **KMS** | Configures display pipeline |
| **Display Engine** | Scans out framebuffer |
| **Monitor** | Displays the pixels |
