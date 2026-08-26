# Linux Bus Subsystems — PCI, USB, I2C, SPI, SoC

The key thing to understand is that PCI, USB, I2C, SPI, and SoC are not really "devices" in the same sense. They are different ways that hardware is connected to the CPU, and Linux has a subsystem for managing each type.

Think of the hierarchy like this:

```
                    CPU / SoC
                       |
        +--------------+--------------+
        |              |              |
       PCI            USB         I2C / SPI
        |              |              |
      Device         Device        Device
```

The reason Linux has different subsystems is that each connection type has different rules for discovering, configuring, and communicating with hardware.

---

## 1. PCI device

Example:

```
CPU
 |
PCIe
 |
 +---- Network card
 +---- NVMe SSD
 +---- GPU
```

PCI/PCIe is commonly used for high-performance peripherals. For example, your computer might have:

```
PCIe → Wi-Fi card
PCIe → Ethernet card
PCIe → NVMe SSD
PCIe → GPU
```

When Linux boots, the PCI subsystem scans the PCI bus and discovers devices.

```
PCI subsystem
      |
      | "What devices are connected?"
      ↓
PCIe device
      |
      ↓
Vendor ID + Device ID
      |
      ↓
Find matching driver
      |
      ↓
Call driver's probe()
```

So when you see "PCI device → PCI subsystem discovers it" it means: Linux can enumerate the PCI bus and find what hardware is attached to it.

---

## 2. USB device

Example:

```
CPU
 |
USB controller
 |
USB hub
 |
 +---- Keyboard
 +---- Mouse
 +---- Webcam
 +---- USB storage
```

USB is designed for external/peripheral devices. A major difference from PCI is that USB devices can frequently be plugged in and removed while the machine is running.

```
             USB subsystem

                  |
                  ↓
             USB device
                  |
          +-------+-------+
          |               |
      Vendor ID       Product ID
          |               |
          +-------+-------+
                  |
                  ↓
             Find driver
                  |
                  ↓
              probe()
```

If you plug in a USB mouse:

```
Plug mouse in
     ↓
USB controller detects it
     ↓
USB subsystem
     ↓
USB device created
     ↓
Matching driver found
     ↓
Driver probe()
```

---

## 3. I2C device

I2C is generally used for small, relatively low-speed peripherals, especially on embedded systems and motherboards.

```
             SoC
              |
             I2C
              |
       +------+-------+
       |              |
 Temperature       EEPROM
   sensor
```

Other examples include: temperature sensors, voltage/current monitors, RTC chips, EEPROMs, touch controllers, power-management chips.

Here's the important difference: **I2C generally doesn't have the same kind of universal discovery mechanism as PCI.** Often the system already knows that a device exists at a particular I2C address.

```
I2C controller
      |
      +---- address 0x48 → temperature sensor
      |
      +---- address 0x50 → EEPROM
```

Linux might learn this information from:

- Device Tree
- ACPI
- board/platform configuration

Then the I2C subsystem creates the appropriate I2C device.

---

## 4. SPI device

SPI is another communication bus commonly used for embedded peripherals.

```
             SoC
              |
             SPI
              |
       +------+------+
       |             |
     Flash         Sensor
```

SPI can be used for: NOR flash, displays, sensors, ADC/DAC devices, touchscreen controllers, other embedded peripherals.

Like I2C, SPI devices generally aren't discovered in the same automatic way PCI devices are. The system often already knows:

```
SPI controller
     |
     +---- Chip Select 0 → Flash
     |
     +---- Chip Select 1 → Sensor
```

Device Tree/ACPI/platform information tells Linux what devices are attached.

---

## 5. SoC device

This one is especially important in embedded Linux. An SoC might contain:

```
             SoC
   +-------------------------+
   |                         |
   | CPU                     |
   |                         |
   | GPIO controller         |
   | I2C controller          |
   | SPI controller           |
   | UART controller         |
   | Timer                   |
   | DMA controller          |
   | PWM controller          |
   | Ethernet controller     |
   |                         |
   +-------------------------+
```

These peripherals are physically inside the SoC. For example:

```
CPU
 |
 +---- GPIO controller
 |
 +---- UART controller
 |
 +---- I2C controller
 |
 +---- SPI controller
 |
 +---- Timer
```

There isn't necessarily a physical "bus scan" that can discover these devices like PCI. Instead, Linux is told about them through things such as Device Tree or ACPI, and they are commonly managed through the platform bus.

---

## The biggest difference: discovery

This is probably the concept you're trying to understand.

### PCI

Linux can ask the PCI hardware: "Who is there?" and enumerate devices.

```
PCI bus
  ↓
Scan
  ↓
Device found
  ↓
Vendor ID / Device ID
  ↓
Driver matching
```

### USB

USB also has defined enumeration mechanisms:

```
USB controller
      ↓
Device connected
      ↓
USB enumeration
      ↓
Device descriptors
      ↓
Driver matching
```

### I2C

Usually Linux is told: "There is an I2C device at address 0x48."

```
Device Tree / ACPI
       ↓
I2C device description
       ↓
I2C subsystem
       ↓
Driver
```

### SPI

Similarly:

```
Device Tree / ACPI
       ↓
SPI device description
       ↓
SPI subsystem
       ↓
Driver
```

### SoC/platform device

Usually:

```
Device Tree / ACPI
       ↓
Platform device
       ↓
Platform bus
       ↓
Driver
```

---

## Why does Linux need all these subsystems?

Because the communication mechanism is different. Imagine you are writing a Linux driver for an NVMe SSD. You don't want to write all the PCIe discovery and configuration logic yourself. Linux already has the PCI subsystem. Your driver can essentially say: "I'm the driver for this PCI device."

Similarly:

```
USB keyboard
       ↓
USB subsystem
       ↓
USB keyboard driver

I2C temperature sensor
       ↓
I2C subsystem
       ↓
Temperature sensor driver

SPI display controller
       ↓
SPI subsystem
       ↓
Display driver

SoC UART
       ↓
Platform subsystem
       ↓
UART driver
```

This allows Linux to separate bus management from device-specific functionality.

---

## A very useful mental model

Think of it like a transportation system.

```
                    Linux Kernel
                         |
          +--------------+--------------+
          |              |              |
        PCI            USB          I2C / SPI
          |              |              |
       Highway        Bus route      Local road
          |              |              |
       Device         Device         Device
```

The subsystem manages the road/transport mechanism. The device driver knows how to operate the actual device. For example:

```
                 Linux
                   |
             PCI subsystem
                   |
             PCIe network card
                   |
            Network driver
                   |
              Ethernet
```

The PCI subsystem handles things like: "How do I communicate with PCIe hardware?"

The network driver handles: "How do I operate this particular network controller?"

---

## And this leads to probe()

This is where the earlier question about device registers becomes connected. A simplified Linux flow looks like:

```
             Hardware
                 |
                 ↓
        Bus / Platform subsystem
                 |
                 ↓
          Device discovered
                 |
                 ↓
          Driver matching
                 |
                 ↓
             probe()
                 |
        +--------+--------+
        |                 |
        ↓                 ↓
   Map/registers       Request IRQ
        |                 |
        ↓                 ↓
   Configure device   Handle events
```

So `probe()` is basically the driver's opportunity to say: "Okay, Linux has told me that my device exists. Now I'll initialize it."

Inside `probe()`, a driver may:

1. Get hardware resources
2. Map device registers
3. Configure the hardware
4. Request an IRQ
5. Initialize buffers/DMA
6. Register the device with another kernel subsystem

That's why understanding this mapping is very important for Linux driver development:

```
PCI → PCI subsystem
USB → USB subsystem
I2C → I2C subsystem
SPI → SPI subsystem
SoC → Device Tree/ACPI → platform bus
```
