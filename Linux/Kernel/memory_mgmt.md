# Operating System - Memory Management Handbook

> Complete interview notes covering memory hierarchy, allocation techniques, paging, segmentation, virtual memory, fragmentation, swapping, and modern OS memory management.

---

# Table of Contents

1. Introduction to Memory Management
2. Why Memory Management is Needed
3. Memory Hierarchy
4. SRAM vs DRAM
5. Responsibilities of Memory Management
6. Memory Allocation Techniques
7. Contiguous Memory Allocation
8. Fixed Partitioning
9. Dynamic Partitioning
10. Memory Allocation Strategies
11. Non-Contiguous Memory Allocation
12. Paging
13. Address Translation in Paging
14. Segmentation
15. Paged Segmentation
16. Virtual Memory
17. Demand Paging
18. Fragmentation
19. Swapping
20. Memory Protection
21. Memory Management Unit (MMU)
22. Memory Management in Modern Operating Systems
23. Advantages of Memory Management
24. Interview Questions

---

# 1. Introduction to Memory Management

Memory Management is one of the most important responsibilities of an Operating System.

It is responsible for:

- Allocating memory to processes
- Tracking memory usage
- Protecting memory
- Reclaiming memory
- Maximizing memory utilization

Without memory management, multiple programs cannot execute safely and efficiently.

---

# 2. What is Memory Management?

Memory is a large collection of bytes (or words) where programs and data are temporarily stored during execution.

Memory Management is the process of:

- Allocating memory
- Tracking allocated memory
- Protecting memory
- Releasing memory

Goal:

- Maximum memory utilization
- Efficient execution
- Fair resource sharing
- Process isolation

---

# 3. Memory Hierarchy

The closer the memory is to the CPU, the faster and more expensive it becomes.

```
               Fastest
        +------------------+
        | Registers        |
        +------------------+
               ↓
        +------------------+
        | L1 Cache         |
        +------------------+
               ↓
        +------------------+
        | L2 Cache         |
        +------------------+
               ↓
        +------------------+
        | L3 Cache         |
        +------------------+
               ↓
        +------------------+
        | Main Memory      |
        | (RAM)            |
        +------------------+
               ↓
        +------------------+
        | SSD / HDD        |
        +------------------+

              Slowest
```

---

## Registers

- Located inside CPU
- Fastest memory
- Very small capacity
- Holds operands and intermediate results

Example

```
R1 = 20
R2 = 30
```

---

## Cache Memory

Stores frequently used instructions and data.

Levels

- L1 Cache
- L2 Cache
- L3 Cache

Characteristics

- Very fast
- Built using SRAM
- Expensive
- Small capacity

---

## Main Memory (RAM)

Stores:

- Running programs
- Process data
- Stack
- Heap

Characteristics

- Volatile
- Built using DRAM
- Larger than cache
- Slower than cache

---

## Secondary Storage

Examples

- SSD
- HDD

Characteristics

- Non-volatile
- Permanent storage
- Used by virtual memory

---

# 4. SRAM vs DRAM

| SRAM | DRAM |
|------|------|
| Static RAM | Dynamic RAM |
| Stores data using flip-flops | Stores data using capacitors |
| No refresh required | Refresh required continuously |
| Faster | Slower |
| Expensive | Cheaper |
| Larger cell size | Smaller cell size |
| Less dense | More dense |
| Used in Cache | Used in Main Memory |

---

## DRAM

Stores every bit as an electrical charge inside a capacitor.

Problem

Charge leaks over time.

Therefore,

Memory must be refreshed thousands of times every second.

Advantages

- Cheap
- High capacity

Used in

- Main Memory (RAM)

---

## SRAM

Stores data using flip-flops.

Characteristics

- No refreshing
- Very fast
- Expensive
- Low capacity

Used in

- L1 Cache
- L2 Cache
- L3 Cache

---

# 5. Responsibilities of Memory Management

The Operating System performs several tasks.

---

## Tracking

Maintains information about

- Free memory
- Allocated memory
- Reserved memory

---

## Allocation

Allocates memory whenever a process requests it.

Example

```
malloc()

new
```

---

## Protection

Ensures

Process A cannot access Process B's memory.

---

## Sharing

Allows multiple processes to safely share memory when required.

Example

Shared Memory IPC.

---

## Relocation

Moves processes in memory when required.

Useful during

- Compaction
- Swapping

---

## Deallocation

Releases memory after process termination.

---

# 6. Memory Allocation Techniques

Two major approaches exist.

```
Memory Allocation

│

├── Contiguous Allocation

└── Non-Contiguous Allocation
```

---

# 7. Contiguous Memory Allocation

Each process occupies one continuous block of memory.

```
+-----------------------+
| Process A             |
+-----------------------+
| Process B             |
+-----------------------+
| Process C             |
+-----------------------+
```

Simple but suffers from fragmentation.

---

# 8. Fixed Partitioning

Also called

```
Static Partitioning
```

---

## Definition

Memory is divided into fixed partitions during system startup.

Each partition contains only one process.

---

## Example

Memory = 1 GB

| Partition | Size |
|-----------|------|
| P1 | 256 MB |
| P2 | 256 MB |
| P3 | 512 MB |

Process

```
200 MB
```

Can fit into

```
P1 or P2
```

Process

```
400 MB
```

Must go into

```
P3
```

---

## Diagram

```
+-------------------+
| Partition 1       |
| 256 MB            |
+-------------------+
| Partition 2       |
| 256 MB            |
+-------------------+
| Partition 3       |
| 512 MB            |
+-------------------+
```

---

## Advantages

- Simple
- Fast allocation
- Low overhead

---

## Disadvantages

### Internal Fragmentation

Unused memory inside allocated partition.

Example

```
Partition = 256 MB

Process = 200 MB

Unused = 56 MB
```

Memory wasted.

---

### Limited Number of Processes

Maximum processes

=

Number of partitions.

---

### Poor Memory Utilization

Large partition assigned to a small process.

---

# 9. Dynamic Partitioning

Also called

```
Variable Partitioning
```

---

## Definition

Memory is allocated according to process size.

Partitions are created dynamically.

---

## Example

Total Memory

```
1024 MB
```

Allocate

```
Process A = 200 MB
```

Remaining

```
824 MB
```

Allocate

```
Process B = 300 MB
```

Remaining

```
524 MB
```

Now

Process A finishes.

```
Free Block = 200 MB
```

Memory becomes

```
200 MB Hole

524 MB Hole
```

New Process

```
250 MB
```

Cannot fit into 200 MB hole.

This causes

External Fragmentation.

---

## Advantages

- Better utilization
- Flexible
- No internal fragmentation

---

## Disadvantages

- External fragmentation
- Compaction required
- Complex allocation algorithms

---

# 10. Memory Allocation Strategies

When multiple free blocks exist, OS chooses one.

---

## First Fit

Choose the first block large enough.

Advantages

- Fast

Disadvantages

- Leaves many small holes.

---

## Best Fit

Choose the smallest block that fits.

Advantages

- Reduces wasted space.

Disadvantages

- Slow search
- Creates many tiny holes

---

## Worst Fit

Choose the largest available block.

Advantages

Leaves large free blocks.

Disadvantages

May waste large memory regions.

---

# 11. Non-Contiguous Memory Allocation

Processes need not occupy consecutive memory locations.

Techniques

- Paging
- Segmentation
- Paged Segmentation

---

# 12. Paging

Paging eliminates the need for contiguous allocation.

Memory is divided into fixed-size blocks.

Logical Memory

↓

Pages

Physical Memory

↓

Frames

---

## Diagram

```
Logical Memory

+------+
|Page0 |
+------+
|Page1 |
+------+
|Page2 |
+------+

↓

Page Table

↓

Physical Memory

+------+
|Frame3|
+------+
|Frame0|
+------+
|Frame5|
+------+
```

Pages can be placed into any free frame.

---

## Advantages

- Eliminates external fragmentation
- Easy allocation
- Efficient virtual memory

---

## Disadvantages

- Small internal fragmentation
- Page table overhead

---

# 13. Address Translation

Logical Address

```
(Page Number, Offset)
```

Page Table

↓

Frame Number

↓

Physical Address

```
(Frame Number, Offset)
```

---

## Example

Logical Memory

```
32 KB
```

Page Size

```
4 KB
```

Number of Pages

```
32 / 4

=

8 Pages
```

Physical Memory

```
16 KB
```

Frames

```
16 / 4

=

4 Frames
```

Suppose

```
Page 0

↓

Frame 2
```

Logical Address

```
(Page 0, Offset 100)
```

Physical Address

```
(Frame 2, Offset 100)
```

Translation performed using

Page Table.

---

# 14. Segmentation

Memory divided according to logical units.

Examples

- Function
- Array
- Stack
- Heap
- Data

Each segment has variable size.

---

## Segment Table

Stores

- Base Address
- Limit

Logical Address

```
Segment Number

+

Offset
```

---

## Advantages

- Logical organization
- Easier programming
- Better protection

---

## Disadvantages

- External fragmentation

---

# 15. Paged Segmentation

Combination of

```
Segmentation

+

Paging
```

Process

```
Segment

↓

Pages

↓

Frames
```

Advantages

- Better protection
- Reduced fragmentation
- Efficient allocation

---

# 16. Virtual Memory

Virtual Memory provides the illusion of larger memory.

Uses

Disk space

+

RAM

---

## Concept

Only required pages remain in RAM.

Remaining pages stay on disk.

```
Program

↓

Virtual Address Space

↓

RAM

↓

Disk
```

---

## Advantages

- Execute large programs
- Better multitasking
- Efficient RAM utilization
- Process isolation

---

## Disadvantages

- Page faults
- Disk access slower than RAM

---

# 17. Demand Paging

Pages are loaded

Only when required.

```
CPU

↓

Needs Page

↓

Page Present?

↓

Yes

↓

Execute

↓

No

↓

Page Fault

↓

Load from Disk

↓

Continue
```

---

## Page Fault

Occurs when requested page is absent from RAM.

OS

- Finds free frame
- Loads page from disk
- Updates page table
- Restarts instruction

---

# 18. Fragmentation

Memory fragmentation reduces memory utilization.

---

## Internal Fragmentation

Unused memory

Inside allocated block.

Example

```
Allocated

256 MB

Used

220 MB

Waste

36 MB
```

Occurs in

- Fixed partitioning
- Paging (last page)

---

## External Fragmentation

Free memory scattered into small holes.

Example

```
100 MB

Free

+

50 MB

Free

+

75 MB

Free
```

Total

225 MB

Process needs

200 MB

Cannot allocate because memory isn't contiguous.

---

## Solutions

### Compaction

Move processes together.

Combine small holes into one large hole.

Disadvantage

Slow.

---

### Paging

Avoids external fragmentation.

---

# 19. Swapping

Swapping moves processes between RAM and disk.

```
RAM

↓

Swap Out

↓

Disk

↓

Swap In

↓

RAM
```

---

## Advantages

- Frees RAM
- Supports more processes
- Improves CPU utilization

---

## Disadvantages

- Disk I/O overhead
- Slower execution

---

# 20. Memory Protection

Memory protection prevents one process from accessing another process's memory.

Methods

- Base Register
- Limit Register
- MMU
- Page Protection
- Segment Protection

Benefits

- Security
- Isolation
- Stability

---

# 21. Memory Management Unit (MMU)

MMU is hardware that translates logical addresses into physical addresses.

```
CPU

↓

Logical Address

↓

MMU

↓

Physical Address

↓

RAM
```

Responsibilities

- Address translation
- Memory protection
- Virtual memory support
- Paging support

---

# 22. Memory Management in Modern Operating Systems

Modern operating systems (Linux, Windows, macOS) use multiple techniques together.

They use

- Paging
- Virtual Memory
- Demand Paging
- Multi-level Cache
- MMU
- Memory Protection
- Copy-on-Write (CoW)
- Page Replacement Algorithms

This provides:

- Better performance
- Better security
- Efficient memory utilization
- Large virtual address space

---

# 23. Advantages of Memory Management

- Efficient memory utilization
- Efficient CPU utilization
- Supports multitasking
- Enables virtual memory
- Provides memory protection
- Process isolation
- Better system performance
- Reduces memory wastage
- Supports larger applications
- Improves overall system stability

---

# 24. Interview Questions

## Basic

- What is memory management?
- Why is memory management required?
- Explain memory hierarchy.
- Difference between SRAM and DRAM.
- What are the responsibilities of memory management?
- What is contiguous memory allocation?
- What is non-contiguous memory allocation?

---

## Intermediate

- Explain fixed partitioning.
- Explain dynamic partitioning.
- Internal vs external fragmentation.
- First Fit vs Best Fit vs Worst Fit.
- What is paging?
- What is a page?
- What is a frame?
- What is a page table?
- Explain logical and physical addresses.
- What is segmentation?
- Paging vs segmentation.

---

## Advanced

- Explain virtual memory.
- What is demand paging?
- What is a page fault?
- How does MMU work?
- Explain swapping.
- What is compaction?
- How does Linux manage memory?
- Why is paging preferred over dynamic partitioning?
- Why is virtual memory slower than RAM?
- Explain modern OS memory management techniques.