# Linux Driver Lifecycle — Complete Flow

## Part 1: End-to-End Driver Lifecycle (Boot → Probe → Runtime → Removal)

```
                              Linux boots
                                  |
                                  ↓
              Kernel initializes frameworks/subsystems
                                  |
        +-------------------------+-------------------------+
        |                         |                         |
        ↓                         ↓                         ↓
   PCI subsystem             USB subsystem           Platform subsystem
        |                         |                         |
        ↓                         ↓                         ↓
   PCI enumeration          USB enumeration         Device Tree / ACPI
        |                         |                         |
        ↓                         ↓                         ↓
   PCI device found         USB device found        Platform device info
   (GPU/NVMe/NIC)           (Keyboard/etc.)         (UART/GPIO/etc.)
        |                         |                         |
        ↓                         ↓                         ↓
   struct pci_dev            struct usb_device       struct platform_device
        |                         |                         |
        +-------------------------+-------------------------+
                                  |
                    +-------------+-------------+
                    |                           |
                    ↓                           ↓
               I2C subsystem               SPI subsystem
                    |                           |
                    ↓                           ↓
              Device Tree/ACPI             Device Tree/ACPI
                    |                           |
                    ↓                           ↓
                I2C device                  SPI device
              (Sensor/RTC)               (Flash/Sensor)
                    |                           |
                    ↓                           ↓
              struct i2c_client             struct spi_device
                    |                           |
                    +-------------+-------------+
                                  |
                                  ↓
                 Each device has a generic
                    struct device inside it
                                  |
                                  ↓
              Driver matching on corresponding bus
                                  |
              +-------------------+-------------------+
              |                                       |
              ↓                                       ↓
          No match                                  Match
              |                                       |
              ↓                                       ↓
      Device remains unbound                    Driver bound
                                                      |
                                                      ↓
                                                   probe()
                                                      |
                                                      ↓
                                             Driver initializes:
                                             - Resources
                                             - Registers
                                             - IRQ
                                             - DMA
                                             - Hardware
                                                      |
                                                      ↓
                                             Register with the
                                           appropriate kernel subsystem
                                                      |
                                                      ↓
                                             probe() succeeds
                                                      |
                                                      ↓
                                             +----------------+
                                             |  DEVICE READY  |
                                             +----------------+
                                                      |
                                                      ↓
                                             NORMAL OPERATION
                                                      |
                              +-----------------------+-----------------------+
                              |                       |                       |
                              ↓                       ↓                       ↓
                       Kernel/App request       Driver ↔ Device          Device event
                              |                 communication                 |
                              ↓                       |                       ↓
                         Subsystem                    |                      IRQ
                              |                       |                       |
                              +-----------+-----------+-----------------------+
                                          |
                                          ↓
                                  Driver handles request
                                          |
                                          ↓
                                  Device continues running
                                          |
                                          |
                         +----------------+----------------+
                         |                                 |
                         ↓                                 ↓
              If character device                  If another interface
                  is needed:                           is used:
                         |                                 |
                         ↓                                 ↓
             alloc_chrdev_region()                 Network / Block /
                         |                         Input / etc. subsystem
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
                  open/read/write/
                     ioctl/close
                         |
                         ↓
                    Driver
                         |
                         ↓
              Hardware registers /
                    DMA / IRQ
                         |
                         ↓
                       Device


                  DEVICE REMOVED / UNBOUND
                              |
                              ↓
                           remove()
                              |
                              ↓
                      Driver cleanup:
                      - Stop hardware
                      - Free IRQ
                      - Free DMA
                      - Unmap registers
                      - Remove cdev
                      - Destroy device
                      - Release resources
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
