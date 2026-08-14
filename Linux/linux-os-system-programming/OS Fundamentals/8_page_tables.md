# Chapter 8 — Page Tables + TLB + Page Faults

> **Three-layer approach**
>
> This chapter covers:
> 1. **[OS] Operating System concepts**
> 2. **[LSP] Linux System Programming + C code**
> 3. **[KERNEL] Linux Kernel Internals**
>
> Chapter 7 introduced virtual memory. This chapter goes one level deeper into the actual address-translation and fault-handling path.

---

# 1. Big Picture

A CPU executes an instruction containing a virtual address.

```text
CPU
 |
 | Virtual Address
 v
MMU
 |
 v
TLB
 |
 +---- HIT ----> Physical Address
 |
 +---- MISS
       |
       v
   Page Table Walk
       |
       +---- Valid ----> Physical Address
       |
       +---- Invalid / needs handling
                    |
                    v
               Page Fault
                    |
                    v
              Linux MM subsystem
                    |
             +------+------+
             |             |
          Resolve       Reject
             |             |
             v             v
       Resume access   SIGSEGV/SIGBUS
```

This is the core flow to understand for interviews.

---

# 2. Virtual Address vs Physical Address

A user program normally works with:

```text
Virtual Address
```

RAM is accessed using:

```text
Physical Address
```

The CPU's memory-management hardware translates between them.

```text
Virtual Address
      |
      v
Translation
      |
      v
Physical Address
```

The OS establishes the mappings; the MMU performs hardware translation during memory access.

---

# 3. Pages and Frames

Virtual memory is divided into:

```text
Pages
```

Physical memory is divided into:

```text
Page Frames
```

For example, with a 4 KiB page:

```text
Virtual:

Page 0
Page 1
Page 2
...

Physical:

Frame 0
Frame 1
Frame 2
...
```

A page-table mapping connects a virtual page to a physical frame.

```text
Virtual Page 25
      |
      v
Physical Frame 800
```

---

# 4. Virtual Address Structure

For a 4 KiB page:

```text
4 KiB = 4096 bytes = 2^12
```

Therefore the lowest 12 bits identify the offset within the page.

Conceptually:

```text
+--------------------------+------------+
| Virtual Page Number      | Page Offset|
+--------------------------+------------+
                           12 bits
```

Example:

```text
VA = 0x12345ABC

Offset = 0xABC
```

The remaining upper portion identifies the virtual page.

---

# 5. Why Page Offset Does Not Change

Suppose:

```text
Virtual Page -> Physical Frame
```

The mapping changes the page number, not the offset.

Example:

```text
Virtual address:

Page 100 + offset 0xABC

        |
        v

Physical frame 500 + offset 0xABC
```

So:

```text
Virtual offset = Physical offset
```

The MMU translates the page-number portion.

---

# 6. Page Table

A page table contains translation information.

Conceptually:

```text
Virtual Page    Physical Frame

0               10
1               25
2               7
3               not present
4               90
```

A real page-table entry contains more than just a frame number.

It can contain information such as:

```text
physical frame number
present/valid state
read/write permission
user/supervisor permission
execute permission
accessed state
dirty state
architecture-specific flags
```

Exact fields depend on the architecture.

---

# 7. Page Table Entry — PTE

A page-table entry is commonly called:

```text
PTE
```

Conceptually:

```text
+--------------------------------+
| Physical Frame Number          |
+--------------------------------+
| Present / Valid                |
| Read / Write                   |
| User / Supervisor              |
| Execute permission             |
| Accessed                       |
| Dirty                          |
| Architecture-specific bits     |
+--------------------------------+
```

The CPU uses the relevant architecture-defined information to determine whether a memory access can proceed.

---

# 8. Why Page Tables Need Permissions

Consider:

```text
Code page
```

It should normally not be writable.

```text
Data page
```

should normally be writable but not necessarily executable.

This gives:

```text
Code       -> R-X
Read-only  -> R--
Data       -> RW-
Stack      -> RW-
```

The exact permissions depend on the mapping.

This helps provide memory protection.

---

# 9. Multi-Level Page Tables

Modern systems generally use multi-level page tables rather than one enormous flat table.

Conceptually:

```text
Virtual Address
      |
      v
+-----------+
| Level 1   |
+-----------+
      |
      v
+-----------+
| Level 2   |
+-----------+
      |
      v
+-----------+
| Level 3   |
+-----------+
      |
      v
+-----------+
| Level 4   |
+-----------+
      |
      v
+-----------+
| PTE       |
+-----------+
      |
      v
Physical Frame
```

The exact number and naming of levels depend on architecture and configuration.

---

# 10. Why Multi-Level Page Tables?

Suppose a process has a huge virtual address space but uses only a few regions.

A flat table could require a very large amount of memory.

Multi-level tables allow the OS to avoid creating every lower-level structure for unused address-space regions.

Conceptually:

```text
Huge virtual address space
          |
          v
Only populate branches that are needed
```

This saves memory.

---

# 11. Page Table Walk

A page-table walk means following the page-table hierarchy to find the mapping for a virtual address.

Conceptually:

```text
Virtual Address
      |
      v
Level 1 index
      |
      v
Level 2 index
      |
      v
Level 3 index
      |
      v
Level 4 index
      |
      v
PTE
      |
      v
Physical frame
```

The page offset is then combined with the physical frame.

---

# 12. TLB

TLB stands for:

```text
Translation Lookaside Buffer
```

It is a hardware cache of recent address translations.

Without a TLB, the CPU may need page-table translation work repeatedly.

With a TLB:

```text
Virtual Page
     |
     v
   TLB
     |
     +---- Hit ----> Physical Frame
     |
     +---- Miss
            |
            v
       Page-table walk
```

---

# 13. TLB Hit

Suppose:

```text
Virtual Page 10
```

is already in the TLB.

```text
VA
 |
 v
TLB
 |
 | HIT
 v
Physical Frame
```

The CPU can use the cached translation without performing the full page-table walk.

---

# 14. TLB Miss

If the translation is not in the TLB:

```text
VA
 |
 v
TLB
 |
 | MISS
 v
Page-table walk
 |
 +---- Valid mapping ----> Physical address
```

A TLB miss is primarily a translation-cache miss.

It is **not automatically a page fault**.

---

# 15. TLB Miss vs Page Fault

This is one of the most important interview distinctions.

```text
TLB MISS
   |
   v
Page-table lookup
   |
   +---- valid mapping ---> continue
   |
   +---- invalid/problem --> page fault
```

Therefore:

```text
TLB miss != page fault
```

A page fault occurs only when the memory access cannot be completed using the current translation/protection state and the CPU enters the fault-handling path.

---

# 16. Page Fault

A page fault is a CPU exception caused when a memory access cannot be completed normally.

Possible causes include:

```text
page not currently mapped
page not resident
copy-on-write write
file-backed page needs to be brought in
swapped-out page
permission violation
invalid virtual address
```

Therefore:

> A page fault is not automatically an error.

---

# 17. Recoverable Page Fault

Example:

```text
Process accesses a valid anonymous page
        |
        v
Page is not currently populated
        |
        v
Page fault
        |
        v
Kernel allocates/provides page
        |
        v
Page table updated
        |
        v
Instruction resumes
```

This is normal virtual-memory operation.

---

# 18. Invalid Page Fault

Example:

```c
int *p = NULL;
*p = 10;
```

Conceptually:

```text
CPU memory access
      |
      v
Page fault
      |
      v
Kernel checks address
      |
      v
No valid user mapping
      |
      v
Signal delivered
```

A typical result is:

```text
SIGSEGV
```

but the exact signal depends on the fault and mapping situation.

---

# 19. Protection Fault

Consider a read-only page:

```text
Page:
R--
```

and code attempts:

```c
*p = 100;
```

The translation may exist, but the requested write is not permitted.

Conceptually:

```text
Page table:
valid mapping
+
read-only permission

        |
        v

CPU performs write
        |
        v
Protection fault
        |
        v
Kernel checks access
        |
        v
Reject / signal
```

---

# 20. Page Fault Does Not Mean "Page Missing"

A page fault can represent several situations:

```text
1. Missing mapping
2. Demand allocation
3. Copy-on-write
4. File-backed fault
5. Swap-in
6. Protection violation
7. Invalid address
```

So avoid saying:

> Page fault simply means page is not in RAM.

That is an incomplete definition.

---

# 21. Page Fault Handler — High-Level Flow

```text
CPU
 |
 | fault
 v
Architecture-specific exception entry
 |
 v
Linux page-fault handling
 |
 v
Identify:
  - faulting address
  - access type
  - current process
  - VMA
  - page-table state
 |
 v
Determine cause
 |
 +--> anonymous memory
 |
 +--> file-backed memory
 |
 +--> COW
 |
 +--> swap
 |
 +--> protection violation
 |
 +--> invalid address
 |
 v
Resolve or reject
 |
 +--> resume process
 |
 +--> signal process
```

Exact function names and internal call paths vary by architecture and Linux kernel version.

---

# 22. Faulting Address

The CPU/kernel needs to know the address that caused the fault.

Conceptually:

```text
Instruction
    |
    v
memory access to VA X
    |
    v
fault
    |
    v
kernel obtains fault address
```

The architecture provides the relevant fault information to the kernel.

Linux then uses the address to locate the relevant virtual memory region.

---

# 23. VMA and Page Fault

Linux uses a VMA to determine whether an address belongs to a valid virtual-memory region.

Conceptually:

```text
Fault address
      |
      v
Find VMA
      |
      +---- found
      |      |
      |      v
      |   Check permissions
      |
      +---- not found
             |
             v
          Invalid access
```

This is why understanding VMA from Chapter 7 is important.

---

# 24. `mm_struct` and Page Tables [KERNEL]

Linux associates a process's user-space memory context with:

```c
struct mm_struct
```

Conceptually:

```text
Process
   |
   v
mm_struct
   |
   +--> virtual memory layout
   +--> VMA information
   +--> page-table context
   +--> memory accounting
```

Architecture-specific page-table structures represent the actual translation hierarchy.

---

# 25. Linux Page-Table Abstractions [KERNEL]

Linux provides architecture-independent page-table abstractions.

Common conceptual levels include:

```text
PGD
P4D
PUD
PMD
PTE
```

Depending on architecture/configuration, some levels may be folded or represented differently.

Do not assume every architecture has identical hardware page-table levels.

---

# 26. PGD

Conceptually:

```text
PGD
 |
 +--> selects a high-level page-table branch
```

Linux uses architecture-independent macros/functions to navigate page-table structures.

The exact hardware interpretation depends on architecture.

---

# 27. P4D

Conceptually:

```text
PGD
 |
 v
P4D
 |
 v
next level
```

Some configurations fold this level.

The important interview point is:

> Linux's generic page-table API can represent a hierarchy even when the underlying architecture does not use every level as a distinct hardware structure.

---

# 28. PUD

Conceptually:

```text
PGD
 |
 v
P4D
 |
 v
PUD
 |
 v
PMD
```

PUD is another level in Linux's page-table abstraction.

---

# 29. PMD

Conceptually:

```text
...
 |
 v
PMD
 |
 v
PTE
```

A PMD can also participate in mappings for larger page sizes where supported.

---

# 30. PTE

The PTE is the level that directly represents the mapping of a base-size virtual page.

Conceptually:

```text
PTE
 |
 +--> physical frame
 +--> permissions
 +--> present/access state
 +--> architecture-specific flags
```

---

# 31. Linux Page-Table Walk — Conceptual

```text
Virtual Address
      |
      v
PGD
      |
      v
P4D
      |
      v
PUD
      |
      v
PMD
      |
      v
PTE
      |
      v
Physical Frame
      +
Page Offset
      |
      v
Physical Address
```

Again, some levels can be folded depending on architecture/configuration.

---

# 32. TLB Makes This Faster

Without TLB:

```text
Every memory access
       |
       v
page-table translation
```

With TLB:

```text
Every memory access
       |
       v
TLB lookup
       |
    +--+--+
    |     |
   hit   miss
    |     |
    |     v
    |  page-table walk
    |     |
    +-----+
          |
          v
    physical address
```

The TLB avoids repeated translation work for recently used pages.

---

# 33. TLB and Locality

Programs often exhibit:

```text
temporal locality
spatial locality
```

Therefore recently used translations are likely to be useful again.

The TLB exploits this locality.

Example:

```c
for (int i = 0; i < 1000000; ++i)
    sum += array[i];
```

The program repeatedly accesses nearby memory pages, so translation caching can be effective.

---

# 34. TLB Coverage

TLB capacity is limited.

Suppose a TLB can effectively cache translations covering:

```text
N pages
```

Larger pages increase the amount of memory covered by each translation.

Therefore huge pages can increase TLB reach.

```text
Small pages:
1 TLB entry -> small memory region

Huge pages:
1 TLB entry -> larger memory region
```

---

# 35. Huge Pages and TLB

Benefits:

```text
larger page
    |
    +--> fewer pages for same memory
    |
    +--> fewer translations needed
    |
    +--> potentially fewer TLB misses
```

Trade-offs:

```text
larger allocation granularity
possible fragmentation
more difficult memory management
```

---

# 36. TLB Invalidation

When a mapping changes, stale TLB entries can become incorrect.

For example:

```text
VA X -> Physical Page A
```

changes to:

```text
VA X -> Physical Page B
```

The old TLB entry must not continue to be used incorrectly.

Therefore the system uses TLB invalidation/shootdown mechanisms.

---

# 37. TLB Shootdown

On multiprocessor systems:

```text
CPU 0
CPU 1
CPU 2
CPU 3
```

multiple CPUs may have cached translations for the same address space.

If a mapping changes, relevant CPUs may need their stale translations invalidated.

Conceptually:

```text
CPU 0 ----+
CPU 1 ----+--> mapping changed
CPU 2 ----+
CPU 3 ----+
            |
            v
      invalidate stale TLB entries
```

This is called a TLB shootdown in the SMP context.

---

# 38. Why TLB Shootdowns Are Expensive

They can involve:

```text
cross-CPU coordination
interrupt/IPI mechanisms
TLB invalidation
synchronization
```

Therefore frequent mapping changes can have performance implications.

This becomes particularly relevant in:

```text
high-performance systems
large multithreaded applications
memory allocators
virtual machines
kernel memory management
```

---

# 39. Address-Space Switching

When switching between processes with different address spaces:

```text
Process A
   |
   v
Process B
```

the CPU must ensure translations are associated with the correct address space.

Architectures provide mechanisms such as:

```text
TLB invalidation
ASID
PCID
```

depending on architecture.

The purpose is to avoid using a translation belonging to the wrong address space.

---

# 40. User/Kernel Page Protection

Page tables can distinguish privilege levels.

Conceptually:

```text
User page
    |
    +--> accessible from user mode

Kernel page
    |
    +--> privileged access
```

If user code attempts an invalid privileged access:

```text
fault
  |
  v
kernel fault handling
  |
  v
reject access
```

This is a fundamental security boundary.

---

# 41. Copy-on-Write Page Fault

COW is one of the most important recoverable page faults.

After:

```c
fork();
```

conceptually:

```text
Parent page table ----+
                      |
                      v
                  Page X
                      ^
                      |
Child page table -----+
```

Both mappings are arranged for COW.

---

# 42. COW Write

Child executes:

```c
*ptr = 42;
```

Flow:

```text
Child write
    |
    v
PTE does not permit normal write
    |
    v
Page fault
    |
    v
Kernel determines COW
    |
    v
Allocate/copy page
    |
    v
Update child PTE
    |
    v
Child retries instruction
```

Result:

```text
Parent -> Page X
Child  -> Page Y
```

---

# 43. Why COW Uses Page Faults

The kernel wants to delay copying until a process actually writes.

Therefore:

```text
fork()
  |
  v
share pages
  |
  v
make writes fault
  |
  v
copy only when necessary
```

This is a classic example of using a page fault as a lazy mechanism.

---

# 44. Anonymous Demand Paging

Consider:

```c
char *p = malloc(4096);
*p = 'A';
```

Conceptually:

```text
malloc()
   |
   v
virtual memory available
   |
   v
*p = 'A'
   |
   v
page fault if page not populated
   |
   v
kernel provides page
   |
   v
PTE established
   |
   v
write completes
```

The first access can therefore have a different cost from later accesses.

---

# 45. File-Backed Page Fault

Suppose:

```c
char *p = mmap(file, ...);
```

Then:

```text
CPU reads p[0]
      |
      v
Page fault if required page isn't currently mapped/resident
      |
      v
Kernel checks file-backed VMA
      |
      v
page-cache/file mapping path
      |
      v
data becomes available
      |
      v
PTE established
      |
      v
instruction resumes
```

---

# 46. Swap-In Fault

If an anonymous page has been moved to swap:

```text
Process accesses page
       |
       v
Page fault
       |
       v
Kernel identifies swapped page
       |
       v
Read page from swap
       |
       v
Install mapping
       |
       v
Resume process
```

This can be a major fault because storage I/O may be required.

---

# 47. Minor vs Major Fault

A useful interview model:

```text
Minor fault:
fault resolved without disk/storage I/O.

Major fault:
fault resolution requires storage I/O.
```

Examples:

```text
Minor:
COW copy
already-available page/cache path
demand-zero page

Major:
swap-in
file data requiring storage read
```

Exact accounting can be more nuanced in Linux.

---

# 48. Page Fault Performance

A page fault can be much more expensive than a normal memory access.

Normal:

```text
CPU
 |
 v
TLB hit
 |
 v
RAM
```

Fault with storage I/O:

```text
CPU
 |
 v
fault
 |
 v
kernel
 |
 v
storage I/O
 |
 v
page available
 |
 v
mapping
 |
 v
resume
```

Storage latency can dominate.

---

# 49. Why Page Faults Are Important for Performance

Frequent faults can indicate:

```text
working-set pressure
poor locality
memory pressure
large mappings
swapping
file access patterns
allocator behavior
```

Performance engineers therefore monitor page-fault activity.

---

# 50. `mmap()` and Page Tables [LSP + KERNEL]

Calling:

```c
mmap(...)
```

does not necessarily mean every PTE is immediately populated with a physical page.

Conceptually:

```text
mmap()
 |
 v
VMA created/updated
 |
 v
virtual address range established
 |
 v
first access
 |
 v
page fault
 |
 v
PTE populated as required
```

This lazy behavior is a key concept.

---

# 51. `mprotect()` and Page Faults

Suppose:

```c
mprotect(addr, size, PROT_READ);
```

Then:

```c
*addr = 10;
```

may generate a protection fault.

Flow:

```text
write
 |
 v
PTE/VMA says read-only
 |
 v
fault
 |
 v
kernel checks access
 |
 v
reject
```

This demonstrates how page-table permissions affect CPU accesses.

---

# 52. Practical C Program — `mprotect()`

```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void)
{
    long page_size = sysconf(_SC_PAGESIZE);

    char *p = mmap(
        NULL,
        page_size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );

    if (p == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }

    p[0] = 'A';

    if (mprotect(p, page_size, PROT_READ) != 0)
    {
        perror("mprotect");
        munmap(p, page_size);
        return 1;
    }

    printf("read: %c\n", p[0]);

    /*
     * This write violates the new protection.
     * It is expected to fault.
     */
    p[0] = 'B';

    munmap(p, page_size);
    return 0;
}
```

Compile:

```bash
gcc mprotect_demo.c -o mprotect_demo
```

The final write is intentionally invalid.

---

# 53. Practical Lab — Inspect Page Size

Use:

```bash
getconf PAGE_SIZE
```

or:

```bash
getconf PAGESIZE
```

Typical output on many Linux systems:

```text
4096
```

But do not hard-code this assumption into portable programs.

In C:

```c
long page_size = sysconf(_SC_PAGESIZE);
```

---

# 54. Practical Lab — Inspect Process Mappings

Run:

```bash
cat /proc/self/maps
```

or:

```bash
cat /proc/<pid>/maps
```

Look for:

```text
[heap]
[stack]
executable
shared libraries
anonymous mappings
```

This shows virtual address ranges, not a direct list of physical RAM frames.

---

# 55. Practical Lab — Inspect Memory Statistics

Use:

```bash
cat /proc/<pid>/status
```

Look at fields such as:

```text
VmSize
VmRSS
VmData
VmStk
VmExe
VmLib
```

Compare:

```text
VmSize
```

with:

```text
VmRSS
```

to reinforce the difference between virtual address-space usage and resident memory.

---

# 56. Practical Lab — Observe Fault Counts

Use:

```bash
/usr/bin/time -v ./program
```

On systems where supported, inspect:

```text
Minor (reclaiming a frame) page faults
Major (requiring I/O) page faults
```

This is useful for comparing programs that:

```text
allocate memory
touch memory
access files
```

---

# 57. Practical Lab — Compare Allocation and Touching

Program:

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    size_t size = 256 * 1024 * 1024;

    char *p = malloc(size);

    if (!p)
    {
        perror("malloc");
        return 1;
    }

    printf("memory allocated\n");

    getchar();

    memset(p, 1, size);

    printf("memory touched\n");

    getchar();

    free(p);
    return 0;
}
```

Run:

```bash
/usr/bin/time -v ./program
```

Observe how the memory behavior changes after touching the pages.

---

# 58. Practical Lab — `fork()` and COW

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    size_t size = 100 * 1024 * 1024;

    char *p = malloc(size);

    if (!p)
        return 1;

    for (size_t i = 0; i < size; i += 4096)
        p[i] = 1;

    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        free(p);
        return 1;
    }

    if (pid == 0)
    {
        for (size_t i = 0; i < size; i += 4096)
            p[i] = 2;

        printf("child modified pages\n");
    }
    else
    {
        wait(NULL);
        printf("parent done\n");
    }

    free(p);
    return 0;
}
```

Conceptually:

```text
Before fork:
Parent -> physical pages

After fork:
Parent ----+
           |
           +--> same pages
           |
Child -----+

Child writes:
Child -> COW pages
Parent -> original pages
```

---

# 59. Important: Page Tables Are Not the TLB

Do not confuse:

```text
Page table
```

with:

```text
TLB
```

Page table:

```text
OS-managed translation structures
```

TLB:

```text
CPU hardware cache of translations
```

Conceptually:

```text
Page table = authoritative translation structure

TLB = fast cached copy of recent translations
```

---

# 60. Important: TLB Is Not Page Cache

They solve different problems.

```text
TLB
 |
 +--> caches address translations
```

```text
Page cache
 |
 +--> caches file contents in memory
```

Therefore:

```text
TLB != page cache
```

---

# 61. Important: Page Table vs VMA

```text
VMA
 |
 +--> describes a virtual address range
```

```text
Page table
 |
 +--> translates virtual pages to physical frames
```

Example:

```text
VMA:
0x10000000 - 0x20000000
RW

Page tables:
some pages -> physical frames
some pages -> not currently populated
```

A VMA can cover a region larger than the set of currently resident physical pages.

---

# 62. Page Table Permissions

Suppose:

```text
PTE:
Present = 1
Write   = 0
User    = 1
```

Then a user-mode read can potentially succeed, while a user-mode write can cause a protection fault.

The actual permissions depend on architecture and other page-table/VMA state.

---

# 63. Accessed and Dirty Bits

Hardware-supported page-table state can include:

```text
Accessed
Dirty
```

Conceptually:

```text
Access page
    |
    v
Accessed bit may become set
```

For writable pages:

```text
Write page
    |
    v
Dirty state may become set
```

Linux can use such information for memory-management decisions.

Exact hardware behavior varies.

---

# 64. Page Reclaim Connection

Suppose the system experiences memory pressure.

```text
Memory pressure
      |
      v
reclaim
      |
      +--> clean file-backed pages
      |
      +--> other reclaimable memory
      |
      +--> write/evict pages as appropriate
```

Later access can generate a page fault to bring required data back.

This connects:

```text
page tables
+
page faults
+
page cache
+
swap
+
reclaim
```

---

# 65. Fault + Page Cache

File-backed mapping:

```text
VMA
 |
 v
file-backed mapping
 |
 v
fault
 |
 v
page cache
 |
 +---- page already cached
 |          |
 |          v
 |       map/use
 |
 +---- page not cached
            |
            v
        storage read
            |
            v
        page cache
            |
            v
        map/use
```

This is a key Linux filesystem/VM connection.

---

# 66. Fault + COW

COW mapping:

```text
Parent + Child
      |
      v
Shared physical page
      |
      v
write attempt
      |
      v
page fault
      |
      v
allocate/copy
      |
      v
update PTE
      |
      v
resume
```

---

# 67. Fault + Swap

Swapped page:

```text
PTE indicates page is not resident
        |
        v
access
        |
        v
page fault
        |
        v
swap-in
        |
        v
physical page
        |
        v
page-table update
        |
        v
resume
```

---

# 68. Page Fault + Signal

Not every fault is recoverable.

Example:

```c
int *p = NULL;
*p = 10;
```

Possible flow:

```text
fault
 |
 v
kernel determines invalid mapping
 |
 v
process cannot legally continue
 |
 v
signal
 |
 v
SIGSEGV
```

For file mappings and some other cases, `SIGBUS` can occur depending on the underlying condition.

---

# 69. Senior Interview Question — Explain TLB

Strong answer:

> The TLB is a hardware cache of recent virtual-to-physical translations. On a memory access, the CPU checks the TLB first. A hit provides the translation quickly. On a miss, the processor performs or triggers a page-table walk. If the page-table state represents a valid mapping, the translation can be used and cached in the TLB. If the access cannot be satisfied, a page fault occurs.

---

# 70. Senior Interview Question — TLB Miss vs Page Fault

Answer:

```text
TLB miss:
translation not present in TLB.

Page fault:
current page-table/protection state cannot satisfy
the memory access and fault handling is required.
```

A TLB miss can be followed by a successful page-table walk without any page fault.

---

# 71. Senior Interview Question — Why Multi-Level Page Tables?

Answer:

> Multi-level page tables reduce memory overhead for sparse address spaces. Instead of allocating a complete flat page table for the entire virtual address space, lower-level structures are created only for portions that are needed.

---

# 72. Senior Interview Question — What Happens on a COW Fault?

Answer:

```text
write to COW page
       |
       v
page fault
       |
       v
kernel identifies COW condition
       |
       v
allocate new physical page
       |
       v
copy contents
       |
       v
update writer's page table
       |
       v
resume instruction
```

The parent can continue using the original page.

---

# 73. Senior Interview Question — What Happens on a File-Backed Fault?

Answer:

> The kernel validates the address against the VMA and mapping, determines that the access belongs to a file-backed region, obtains the required page through the file/page-cache path, establishes the appropriate page-table mapping and resumes execution. If storage I/O is required, the fault can become a major fault.

---

# 74. Senior Interview Question — Why Is a Page Fault Expensive?

Answer:

Because handling can involve:

```text
CPU exception
kernel entry
VMA/page-table checks
page allocation
copying
page-cache lookup
storage I/O
TLB/page-table updates
```

The cost depends heavily on the type of fault.

A storage-backed major fault can be orders of magnitude more expensive than a normal memory access.

---

# 75. Senior Interview Question — What Is a TLB Shootdown?

Answer:

> On a multiprocessor system, multiple CPUs can cache translations for an address space. When a mapping changes, stale translations on other CPUs may need to be invalidated. Coordinating these invalidations is commonly called a TLB shootdown.

---

# 76. Senior Interview Question — Why Are Huge Pages Useful?

Answer:

> Huge pages cover more virtual memory per translation. This increases TLB reach and can reduce the number of TLB entries/page-table work required for large memory regions. The trade-offs include larger allocation granularity and potential fragmentation.

---

# 77. Senior Interview Question — Explain the Complete Translation

Strong answer:

```text
CPU generates virtual address
        |
        v
TLB lookup
        |
   +----+----+
   |         |
  HIT       MISS
   |         |
   |         v
   |    page-table walk
   |         |
   |    +----+----+
   |    |         |
   |  valid     invalid
   |    |         |
   +----+         v
        |       fault
        |         |
        |         v
        |    Linux MM handles
        |         |
        |    resolve/reject
        |         |
        +---------+
                  |
                  v
          physical address
                  |
                  v
                 RAM
```

---

# 78. Senior Interview Question — Explain `fork()` + Page Fault

Answer:

```text
fork()
  |
  v
parent/child share physical pages
  |
  v
COW permissions
  |
  v
child writes
  |
  v
page fault
  |
  v
kernel copies page
  |
  v
child PTE points to new page
  |
  v
instruction retries
```

This combines process management and virtual memory.

---

# 79. Senior Interview Question — Why Doesn't `mmap()` Allocate All RAM Immediately?

Answer:

> `mmap()` primarily establishes a virtual mapping. Physical page population can be lazy. When the process accesses a page for the first time, a page fault can cause the kernel to allocate, locate or load the required physical page and establish the mapping.

---

# 80. Senior Interview Question — Can a Page Fault Be Good?

Yes.

Examples:

```text
demand-zero page
COW
file-backed demand paging
swap-in
```

A page fault can be a normal mechanism used to implement lazy memory management.

---

# 81. Senior Interview Question — Can a TLB Miss Be Cheap?

Yes, relative to a page fault.

A TLB miss can simply cause:

```text
page-table walk
 |
 v
valid mapping
 |
 v
translation cached
```

No storage I/O or page allocation is necessarily required.

---

# 82. Common Mistakes

### Mistake 1

> TLB stores physical pages.

Correct:

> TLB caches translations, not the page contents themselves.

### Mistake 2

> Page table is stored in the TLB.

Correct:

> TLB is a cache of translation information derived from page tables.

### Mistake 3

> Every page fault means swapping.

Correct:

> Page faults have many causes; most do not necessarily involve swap.

### Mistake 4

> Every page fault causes SIGSEGV.

Correct:

> Recoverable faults are handled by the kernel; invalid accesses may result in signals.

### Mistake 5

> `mmap()` immediately allocates physical memory for every byte.

Correct:

> Virtual mapping and physical-page population are separate concepts.

---

# 83. One-Minute Revision

```text
Virtual Address
      |
      v
     TLB
      |
  +---+---+
  |       |
 HIT     MISS
  |       |
  |       v
  |   Page-table walk
  |       |
  |    +--+--+
  |    |     |
  |  valid invalid
  |    |     |
  |    |     v
  |    |   Page fault
  |    |     |
  |    |     v
  |    |  Linux MM
  |    |     |
  |    |  resolve/reject
  |    |     |
  +----+-----+
       |
       v
Physical Address
       |
       v
      RAM
```

Remember:

```text
TLB = translation cache
Page table = translation structure
VMA = virtual region description
Page fault = exception for an access that cannot currently complete
PTE = page-level translation/protection entry
COW = write-triggered page fault + private copy
```

---

# 84. Chapter 8 Final Cheat Sheet

| Concept | Meaning |
|---|---|
| Virtual address | Address used by process/CPU instruction |
| Physical address | Address in physical memory |
| Page | Fixed-size virtual-memory unit |
| Frame | Fixed-size physical-memory unit |
| Page table | Maps virtual pages to physical frames |
| PTE | Page-level page-table entry |
| VMA | Linux description of a virtual address range |
| TLB | Hardware cache of address translations |
| TLB hit | Translation found in TLB |
| TLB miss | Translation absent from TLB |
| Page fault | CPU exception requiring fault handling |
| Minor fault | Fault resolved without storage I/O |
| Major fault | Fault involving storage I/O |
| COW | Copy page only when a write requires it |
| TLB shootdown | Cross-CPU invalidation of stale translations |
| Huge page | Larger page used to increase TLB reach |
| `mmap()` | Creates a virtual memory mapping |
| `mprotect()` | Changes memory protection |
| `fork()` | Creates process using COW optimizations |
| `mm_struct` | Linux process memory-management context |
| PGD/P4D/PUD/PMD/PTE | Linux page-table abstraction levels |

---

# 85. Final Mental Model

For senior Linux/system interviews, remember this single chain:

```text
PROCESS
   |
   v
mm_struct
   |
   v
VMA
   |
   v
Virtual Address
   |
   v
TLB
   |
   +------ HIT ------+
   |                 |
   |                 v
   |          Physical Address
   |
   +------ MISS
             |
             v
        Page-table walk
             |
             v
            PTE
             |
       +-----+-----+
       |           |
     valid       fault
       |           |
       v           v
 Physical       Linux MM
   Frame           |
                   +--> COW
                   +--> anonymous page
                   +--> file-backed page
                   +--> swap-in
                   +--> invalid access
                   |
                   v
             update mapping
                   |
                   v
             resume execution
```

---

# 86. Chapter 8 Takeaways

1. The CPU normally generates virtual addresses.
2. The MMU translates virtual addresses using page-table information.
3. Pages are virtual-memory units; frames are physical-memory units.
4. The page offset is preserved during translation.
5. PTEs contain translation and protection information.
6. Modern systems use hierarchical page tables.
7. Linux exposes generic abstractions such as PGD, P4D, PUD, PMD and PTE.
8. Some page-table levels may be folded depending on architecture/configuration.
9. The TLB caches recent translations.
10. A TLB miss is not the same as a page fault.
11. A page fault is a CPU exception requiring OS fault handling.
12. Page faults can be normal and recoverable.
13. Invalid accesses can result in signals such as `SIGSEGV`.
14. COW uses page faults to delay physical copying until a write.
15. File-backed mappings can fault through the page-cache/filesystem path.
16. Swapped pages can cause faults requiring storage I/O.
17. Minor faults generally do not require storage I/O.
18. Major faults generally require storage I/O.
19. Page-table permissions provide memory protection.
20. TLB invalidation is required when translations become stale.
21. SMP systems may require TLB shootdowns.
22. Huge pages increase TLB reach.
23. `mmap()` establishes virtual mappings; physical population can be lazy.
24. `mprotect()` can cause later accesses to fault if permissions are violated.
25. `mm_struct` represents the process memory-management context.
26. VMA describes a virtual address region.
27. Page tables represent the actual translation/protection state used by the MMU.
28. Page cache and virtual memory are closely connected for file-backed mappings.
29. Understanding the translation path is essential for Linux performance debugging.
30. The complete mental model is:

```text
Virtual Address
      ↓
     TLB
      ↓
Page-table walk
      ↓
    PTE
      ↓
Physical Frame
```

and when translation/access cannot proceed:

```text
Page Fault
     ↓
Linux Memory Management
     ↓
Resolve or Signal
```

---

# Chapter 9 Preview — Memory Allocation

Next chapter will focus on **memory allocation**, connecting:

```text
malloc()
   ↓
userspace allocator
   ↓
brk() / mmap()
   ↓
virtual address space
   ↓
pages
   ↓
physical memory
```

It will cover:

```text
heap
malloc/free
calloc/realloc
brk/sbrk
mmap
glibc allocator concepts
arenas
tcache
fragmentation
internal/external fragmentation
memory pools
slab/slub
kmalloc/vmalloc
kernel allocation
GFP flags
page allocator
OOM
memory debugging
```

with C/C++ examples and Linux kernel internals.
