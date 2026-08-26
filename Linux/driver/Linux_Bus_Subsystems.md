# Linux Driver Lifecycle — Complete Flow

## Part 1: End-to-End Driver Lifecycle (Boot → Probe → Runtime → Removal)

```
                                                   LINUX BOOT
                             |
                             ↓
                Initialize kernel subsystems
                             |
        +--------------------+--------------------+
        |                    |                    |
        ↓                    ↓                    ↓
       PCI                  USB              Platform
        |                    |                    |
   Enumeration          Enumeration        DT / ACPI
        |                    |                    |
        ↓                    ↓                    ↓
   PCI device           USB device       Platform device
        |                    |                    |
        ↓                    ↓                    ↓
 struct pci_dev       struct usb_device  struct platform_device
        |                    |                    |
        +--------------------+--------------------+
                             |
                      I2C / SPI devices
                             |
                             ↓
                    Device representation
                  (contains struct device)
                             |
                             ↓
                    DRIVER MATCHING
                             |
                  +----------+----------+
                  |                     |
               No match               Match
                  |                     |
                  ↓                     ↓
              Unbound                Driver
                                        |
                                        ↓
                                      probe()
                                        |
                                        ↓
                              Initialize hardware
                              - Registers
                              - IRQ
                              - DMA
                              - Resources
                                        |
                                        ↓
                                Register with
                             kernel subsystem/interface
                                        |
                                        ↓
                                  DEVICE READY
                                        |
                                        ↓
                               NORMAL OPERATION
                                        |
                         +--------------+--------------+
                         |                             |
                         ↓                             ↓
                  Character device              Other interface
                         |                    Network / Block /
                         |                    Input / etc.
                         ↓
               alloc_chrdev_region()
                         |
                         ↓
                    Major + Minor
                         |
                         ↓
                     cdev_init()
                         |
                         ↓
                      cdev_add()
                         |
                         ↓
                    class_create()
                         |
                         ↓
                    device_create()
                         |
                         ↓
                   /dev/mydevice
                         |
                         ↓
                     Application
                         |
                         ↓
                open/read/write/ioctl
                         |
                         ↓
                       Driver
                         |
                         ↓
                  Hardware
                  /   |   \
              Registers DMA IRQ


              DEVICE REMOVED / UNBOUND
                         |
                         ↓
                      remove()
                         |
                         ↓
                    Driver cleanup
                         |
                         ↓
                   Device removed

```

---

## Part 2: Character Device Registration — Step by Step

```
                         KERNEL
                           |
                           |
                           |  1. Allocate device number
                           |     - Reserve major/minor
                           |     - Major identifies driver
                           |     - Minor identifies instance
                           ↓
                alloc_chrdev_region()
                           |
                           ↓
                    +------------+
                    |   240 : 0  |
                    |------------|
                    | Major Minor|
                    +------------+
                           |
                           |  2. Create/initialize cdev
                           |     - Initialize struct cdev
                           |     - Associate cdev with
                           |       file_operations
                           ↓
                       cdev_init()
                           |
                           ↓
                    +------------+
                    | struct cdev|
                    |------------|
                    | my_cdev    |
                    +------------+
                           |
                           |  3. Register cdev
                           |     - Tell kernel:
                           |       "240:0 is handled
                           |        by my_cdev"
                           ↓
                       cdev_add()
                           |
                           ↓
                +--------------------+
                | Kernel knows:      |
                |                    |
                | 240:0              |
                |   ↓                |
                | my_cdev            |
                +--------------------+
                           |
                           |  4. Connect cdev to
                           |     driver's callbacks
                           |     (done by cdev_init)
                           ↓
                    +------------+
                    |  my_fops   |
                    |------------|
                    | open       |
                    | read       |
                    | write      |
                    | ioctl      |
                    | release    |
                    +------------+
                     /     |      \
                    ↓      ↓       ↓
                 open()  read()   write()
                    |      |       |
                    ↓      ↓       ↓
                my_open my_read my_write
                    |      |       |
                    +------+-------+
                           |
                           ↓
                         DRIVER
                           |
                           |  5. Driver talks to
                           |     physical hardware
                           |
                 +---------+---------+
                 |         |         |
                 ↓         ↓         ↓
             Registers    DMA       IRQ
                 |         |         |
                 +---------+---------+
                           |
                           ↓
                       HARDWARE
```

---

## Part 3: Device-Model Layer — class_create() → device_create() → /dev

```
                    KERNEL DEVICE MODEL
                           |
                           |
                           |  6. Create a logical
                           |     device class
                           ↓
                    class_create()
                           |
                           | Creates class such as:
                           |
                           ↓
                 /sys/class/mydevice/
                           |
                           |
                           |  7. Create a device-model
                           |     device associated with
                           |     major:minor 240:0
                           ↓
                    device_create()
                           |
                           ↓
              +-------------------------+
              | Device-model entry      |
              |                         |
              | mydevice                |
              | dev = 240:0             |
              +-------------------------+
                           |
                           |
                           |  8. Device node is created/
                           |     managed through devtmpfs
                           |     and/or udev
                           ↓
                    /dev/mydevice
                           |
                           |  9. Application opens
                           |     the pathname
                           ↓
                      Application
                           |
                           ↓
                open/read/write/ioctl
                           |
                           ↓
                          VFS
                           |
                           ↓
                       240 : 0
                           |
                           ↓
                        my_cdev
                           |
                           ↓
                        my_fops
                           |
                           ↓
                         DRIVER
                           |
                           ↓
                       HARDWARE
```
