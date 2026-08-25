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
