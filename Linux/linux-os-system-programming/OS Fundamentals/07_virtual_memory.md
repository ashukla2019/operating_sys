# Chapter 7 — Virtual Memory

> **Three-layer approach**
>
> This chapter covers:
> 1. **[OS] Operating System concepts**
> 2. **[LSP] Linux System Programming + C/C++ code**
> 3. **[KERNEL] Linux Kernel Internals**
>
> Goal: understand how a process gets a virtual address space, how virtual addresses are translated to physical memory, why virtual memory exists, how Linux uses `mmap()`, `malloc()`, demand paging and copy-on-write, and how this connects to page tables, TLBs and page faults.

---

# 1. What Is Virtual Memory? [OS]

Virtual memory is a memory-management mechanism that gives each process the illusion of having its own large, private and contiguous address space.

A process uses:

```text
Virtual Address
      |
      v
Memory Management Unit (MMU)
      |
      v
Physical Address
      |
      v
RAM
```

The process normally does **not** directly manipulate physical RAM addresses.

---

# 2. Why Do We Need Virtual Memory?

Virtual memory provides several important benefits:

```text
1. Process isolation
2. Efficient RAM usage
3. Address-space abstraction
4. Protection
5. Shared memory
6. Demand paging
7. Copy-on-write
8. Memory mapping
9. Ability to use more virtual address space than currently resident RAM
```

Without virtual memory, implementing safe independent processes would be much harder.

---

# 3. Process Isolation

Consider:

```text
Process A
virtual address 0x1000
```

and:

```text
Process B
virtual address 0x1000
```

Both processes can use the same virtual address while mapping it to different physical memory.

```text
Process A                    Process B

VA 0x1000                    VA 0x1000
    |                             |
    v                             v
Physical Page X               Physical Page Y
```

Therefore one process normally cannot directly access another process's private memory.

---

# 4. Virtual Address Space

A process has a virtual address space containing regions such as:

```text
High addresses
+----------------------+
| Kernel address space |  (layout depends on architecture/config)
+----------------------+
| Stack                |
+----------------------+
| Shared libraries     |
+----------------------+
| mmap region          |
+----------------------+
| Heap                 |
+----------------------+
| BSS                  |
+----------------------+
| Data                 |
+----------------------+
| Read-only data       |
+----------------------+
| Text / code          |
+----------------------+
Low addresses
```

The exact layout is architecture-, ABI-, kernel- and configuration-dependent.

Do not memorize exact addresses.

---

# 5. Process Memory Segments

Typical executable-related regions:

```text
Text
Read-only data
Data
BSS
Heap
Memory mappings
Stack
```

## Text

Contains executable machine code.

Usually:

```text
read + execute
```

and normally not writable.

## Read-only data

Examples:

```c
const char *msg = "hello";
```

String literals are commonly placed in read-only mapped storage.

## Data

Initialized writable global/static variables.

```c
int global = 10;
```

## BSS

Zero-initialized or uninitialized global/static storage.

```c
int global;
static int count;
```

## Heap

Dynamic memory used by allocation mechanisms.

```c
malloc()
calloc()
realloc()
```

## Stack

Used for:

```text
function calls
local variables
return information
saved registers
```

---

# 6. Virtual Address vs Physical Address

Virtual address:

```text
Used by the CPU/program
```

Physical address:

```text
Address in physical memory
```

Example:

```text
Virtual address:
0x7f1234501000

        |
        v
      MMU
        |
        v

Physical address:
0x0000001234501000
```

The actual mapping is determined by page tables and hardware translation rules.

---

# 7. Pages and Frames

Virtual memory is usually divided into fixed-size **pages**.

Physical memory is divided into fixed-size **frames**.

Example:

```text
Virtual memory:

Page 0
Page 1
Page 2
Page 3
...


Physical memory:

Frame 0
Frame 1
Frame 2
Frame 3
...
```

A virtual page can map to a physical frame.

```text
Virtual Page 5
      |
      v
Physical Frame 21
```

---

# 8. Page Size

Common Linux systems use:

```text
4 KiB
```

as a base page size, but other page sizes and huge-page mechanisms exist.

For a 4 KiB page:

```text
4096 bytes = 2^12
```

Therefore:

```text
offset = lower 12 bits
```

of a simple single-level virtual address decomposition.

---

# 9. Virtual Address Decomposition

For a 4 KiB page:

```text
Virtual Address
+-----------------------+
| Virtual Page Number   | Offset |
+-----------------------+
                        12 bits
```

The offset identifies the byte within the page.

Example:

```text
Virtual address = 0x12345ABC

Page size = 4096 bytes

Offset = lower 12 bits
      = 0xABC
```

The remaining upper bits identify the virtual page number.

---

# 10. Why Use Pages?

Pages provide a convenient unit for:

```text
mapping
protection
paging
sharing
copy-on-write
swapping
demand allocation
```

Instead of managing every byte individually, the OS can manage memory in page-sized units.

---

# 11. Page Table

A page table maps virtual pages to physical frames.

Conceptually:

```text
Virtual Page       Physical Frame

0       ----------> 10
1       ----------> 7
2       ----------> 19
3       ----------> not present
4       ----------> 3
```

The CPU/MMU uses this information when translating addresses.

---

# 12. Page Table Entry

A page-table entry contains information about a mapping.

Conceptually:

```text
+-------------------------------+
| Physical frame number         |
+-------------------------------+
| Present                       |
| Read/Write                    |
| User/Supervisor               |
| Execute permissions           |
| Accessed                      |
| Dirty                         |
| Other architecture flags     |
+-------------------------------+
```

Exact fields depend on the CPU architecture.

---

# 13. Multi-Level Page Tables

Modern systems use multi-level page tables.

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
Physical Frame
```

The exact number of levels depends on architecture and configuration.

---

# 14. Why Multi-Level Page Tables?

A huge flat page table would waste memory.

Most processes do not use every possible virtual address.

Multi-level tables allow unused regions to avoid requiring fully populated lower-level structures.

Conceptually:

```text
Huge virtual address space
          |
          v
only populate structures
for regions actually needed
```

---

# 15. Virtual Memory Is Not the Same as Swap

Important distinction:

```text
Virtual memory
    =
address-space abstraction + translation + memory-management mechanisms
```

Swap:

```text
storage used to move some memory contents
out of RAM when appropriate
```

A system can have virtual memory without relying on swap for every page.

---

# 16. Demand Paging

Demand paging means a page is brought into the required resident state when it is actually needed rather than necessarily being populated eagerly.

Conceptually:

```text
Program accesses virtual address
          |
          v
Page not currently available/resident
          |
          v
Page fault
          |
          v
Kernel handles fault
          |
          v
Make required page available
          |
          v
Instruction can continue
```

This is a key mechanism behind efficient virtual memory.

---

# 17. Page Fault

A page fault occurs when a memory access cannot be completed using the current page-table state and the CPU transfers control to the kernel's fault-handling path.

Important:

> A page fault is not automatically an error.

Many page faults are normal.

Examples include:

```text
demand allocation
copy-on-write
file-backed page not yet resident
swapped-out page
```

An invalid access can also result in a fault that ultimately becomes a process-visible signal such as `SIGSEGV`.

---

# 18. Minor vs Major Page Fault

Linux distinguishes page-fault categories for accounting.

A **minor fault** generally means the fault can be resolved without requiring disk I/O.

A **major fault** generally involves waiting for I/O, such as bringing data from storage.

Conceptually:

```text
Minor:
fault -> memory already available -> map/use

Major:
fault -> storage I/O -> data becomes available -> map/use
```

---

# 19. Demand-Zero / Anonymous Memory

Anonymous memory has no ordinary file backing.

Examples:

```c
malloc()
```

or anonymous:

```c
mmap(MAP_ANONYMOUS, ...)
```

The virtual address range can exist before every physical page has been populated.

A first access may trigger a fault and result in a zero-filled physical page being supplied.

---

# 20. Copy-on-Write

Copy-on-write (COW) is an important virtual-memory optimization.

After:

```c
fork();
```

the parent and child initially can share physical pages under appropriate protection.

Conceptually:

```text
Parent VA ----+
              |
              v
          Physical Page
              ^
              |
Child VA -----+
```

Pages are protected so that a write triggers a fault.

---

# 21. Copy-on-Write Write Path

```text
Parent and child share page
          |
          v
Both mappings initially protected for COW
          |
          v
Child writes
          |
          v
Page fault
          |
          v
Kernel allocates/copies page
          |
          v
Child gets private writable page
```

Now:

```text
Parent -> Page A
Child  -> Page B
```

---

# 22. Why Copy-on-Write Is Useful

Without COW, `fork()` would need to immediately copy all writable memory.

That could be expensive.

With COW:

```text
fork()
  |
  +--> share pages initially
  |
  +--> copy only when modified
```

This is especially valuable when:

```c
fork();
exec(...);
```

because `exec()` replaces the child's address space, making unnecessary copying wasteful.

---

# 23. `fork()` and Virtual Memory [LSP]

Example:

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    int *value = malloc(sizeof(int));

    if (value == NULL)
        return 1;

    *value = 100;

    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        free(value);
        return 1;
    }

    if (pid == 0)
    {
        *value = 200;
        printf("Child:  %d\n", *value);
    }
    else
    {
        wait(NULL);
        printf("Parent: %d\n", *value);
    }

    free(value);
    return 0;
}
```

Expected concept:

```text
Before fork:

Parent -> same physical page

After fork:

Parent -> Page A
Child  -> Page A

Child writes:

Parent -> Page A
Child  -> Page B
```

The virtual address can remain the same in both processes.

---

# 24. `mmap()` [LSP]

`mmap()` creates a mapping in a process's virtual address space.

Basic prototype:

```c
void *mmap(
    void *addr,
    size_t length,
    int prot,
    int flags,
    int fd,
    off_t offset
);
```

Typical uses:

```text
anonymous memory
file mapping
shared memory
shared libraries
large allocations
device mappings
```

---

# 25. Anonymous `mmap()` Example

```c
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void)
{
    size_t size = 4096;

    int *p = mmap(
        NULL,
        size,
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

    *p = 123;

    printf("%d\n", *p);

    munmap(p, size);

    return 0;
}
```

Compile:

```bash
gcc mmap_demo.c -o mmap_demo
```

---

# 26. `mmap()` Working

Conceptually:

```text
mmap()
  |
  v
Kernel creates/updates virtual memory area
  |
  v
Virtual address range becomes available
  |
  v
Physical pages may be populated lazily
  |
  v
First access
  |
  v
Page fault if required
  |
  v
Kernel establishes required mapping
```

The important point:

> Creating a virtual mapping and immediately allocating/populating every physical page are not necessarily the same operation.

---

# 27. `munmap()`

`munmap()` removes a mapping from the process virtual address space.

```c
munmap(address, length);
```

Conceptually:

```text
Virtual mapping
      |
      v
munmap()
      |
      v
mapping removed
```

---

# 28. File-Backed `mmap()`

A file can be mapped into memory.

```text
File
 |
 v
mmap()
 |
 v
Virtual address range
 |
 v
Process accesses memory
 |
 v
Page fault as necessary
 |
 v
Kernel obtains file data
 |
 v
Physical page mapped
```

This is an important connection between:

```text
virtual memory
+
filesystem
+
page cache
```

---

# 29. File Mapping Example [LSP]

```c
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void)
{
    int fd = open("data.txt", O_RDONLY);

    if (fd < 0)
    {
        perror("open");
        return 1;
    }

    struct stat st;

    if (fstat(fd, &st) < 0)
    {
        perror("fstat");
        close(fd);
        return 1;
    }

    if (st.st_size == 0)
    {
        close(fd);
        return 0;
    }

    char *data = mmap(
        NULL,
        st.st_size,
        PROT_READ,
        MAP_PRIVATE,
        fd,
        0
    );

    if (data == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        return 1;
    }

    write(STDOUT_FILENO, data, st.st_size);

    munmap(data, st.st_size);
    close(fd);

    return 0;
}
```

---

# 30. `malloc()` and Virtual Memory [LSP]

A common misconception is:

```text
malloc()
    =
immediately allocate physical RAM
```

That is too simplistic.

A userspace allocator manages virtual address space and obtains memory from the kernel using mechanisms such as:

```text
brk()/sbrk()
mmap()
```

depending on allocation size, allocator implementation, configuration and runtime behavior.

Physical page population may happen lazily.

---

# 31. `malloc()` Conceptual Flow

```text
malloc(size)
     |
     v
Userspace allocator
     |
     +---- existing free heap space?
     |          |
     |         yes
     |          |
     |          v
     |       return
     |
     +---- need more memory
                |
                v
        kernel memory interface
                |
                v
       virtual address space
                |
                v
       physical pages populated
       as required
```

Do not assume every `malloc()` directly results in one system call.

---

# 32. Heap and `brk()` [LSP]

Historically, the process heap can be extended using:

```c
brk()
sbrk()
```

Modern allocators may combine multiple mechanisms and policies.

Important interview point:

> `malloc()` is a library allocator, not a system call.

It may use system calls such as `brk()` and `mmap()` internally.

---

# 33. Stack Growth

A thread's stack occupies a virtual address range.

Stack memory may be populated as needed.

Conceptually:

```text
Function call
     |
     v
Stack access
     |
     v
Required page not resident/mapped
     |
     v
Page fault
     |
     v
Kernel grows/provides stack mapping when valid
```

There are guard-page and limit mechanisms to detect invalid growth.

---

# 34. Stack Overflow

A stack overflow can occur when a thread exceeds its allowed stack space.

Example:

```c
void recurse(void)
{
    recurse();
}
```

Eventually the stack reaches a protected boundary/limit and an invalid access can occur.

This is different from heap exhaustion.

---

# 35. Memory Protection [OS]

Virtual memory also provides permissions.

Typical permissions:

```text
R = read
W = write
X = execute
```

Examples:

```text
Code:
R-X

Read-only data:
R--

Writable data:
RW-

Heap:
RW-

Stack:
RW-
```

Modern systems generally enforce a non-executable writable-memory model where possible.

---

# 36. Why W^X/NX Matters

A writable page can contain data.

An executable page can contain code.

Allowing arbitrary:

```text
RWX
```

memory can increase security risk.

Hardware and OS protections can mark pages non-executable.

The exact policy varies by architecture, OS and mapping.

---

# 37. User Space vs Kernel Space

A process typically has:

```text
User virtual address space
+
Kernel virtual address space
```

The exact layout depends on architecture and kernel configuration.

User code cannot normally access arbitrary kernel virtual memory.

CPU privilege mechanisms enforce this separation.

---

# 38. Kernel Page Tables [KERNEL]

Linux maintains architecture-specific page-table structures representing virtual-to-physical mappings.

The memory-management subsystem coordinates:

```text
virtual memory areas
page tables
physical pages
page cache
reclaim
swap
fault handling
```

Architecture-specific code performs the hardware-facing page-table operations.

---

# 39. `mm_struct` [KERNEL]

Linux associates a process's user-space memory-management information with:

```c
struct mm_struct
```

Conceptually it represents the process address-space memory context.

It is associated with information about:

```text
address-space layout
memory mappings
page-table root/context
VM areas
memory accounting
```

A thread group generally shares an `mm_struct`, while kernel threads typically do not have a normal userspace memory context.

---

# 40. `vm_area_struct` / VMA [KERNEL]

Linux represents contiguous virtual-memory regions using VMAs.

Conceptually:

```text
Process address space

+-----------------------+
| VMA: executable code  |
+-----------------------+
| VMA: read-only data   |
+-----------------------+
| VMA: heap             |
+-----------------------+
| VMA: mmap region      |
+-----------------------+
| VMA: stack            |
+-----------------------+
```

A VMA describes properties of a virtual address range.

---

# 41. VMA Properties

A VMA can represent:

```text
start/end virtual addresses
permissions
file-backed mapping information
anonymous mapping
flags
mapping relationships
```

Think:

```text
VMA = description of a virtual address region
```

It is not the physical memory itself.

---

# 42. VMA vs Page Table

Very important:

```text
VMA
 |
 +--> describes virtual region
```

while:

```text
Page table
 |
 +--> translates virtual pages to physical frames
```

Example:

```text
VMA:
0x400000 - 0x410000
R-X

Page table:
virtual page 0 -> physical frame 100
virtual page 1 -> physical frame 205
...
```

A VMA can exist even when all physical pages in that range are not currently populated.

---

# 43. Page Fault Path [KERNEL]

High-level path:

```text
CPU executes memory access
        |
        v
MMU translation
        |
        v
translation/protection problem
        |
        v
CPU raises page fault
        |
        v
architecture-specific fault entry
        |
        v
Linux fault handling
        |
        v
identify VMA/access
        |
        v
determine cause
        |
        +--> anonymous demand allocation
        |
        +--> file-backed page
        |
        +--> COW
        |
        +--> swapped page
        |
        +--> invalid access
```

Exact kernel function paths vary by architecture and kernel version.

---

# 44. Page Fault Types Conceptually [KERNEL]

A fault may result from:

```text
1. Missing page
2. Copy-on-write write
3. File-backed page not resident
4. Swapped-out page
5. Protection violation
6. Invalid address
```

The kernel examines:

```text
faulting address
access type
VMA
page-table state
mapping
permissions
```

before deciding how to handle it.

---

# 45. Invalid Memory Access

Example:

```c
int *p = NULL;
*p = 10;
```

The CPU attempts an invalid access.

Typical sequence:

```text
invalid virtual address
       |
       v
page fault
       |
       v
kernel checks address
       |
       v
no valid mapping
       |
       v
SIGSEGV
```

The exact signal and behavior depend on the fault.

---

# 46. Page Cache Connection [KERNEL]

File-backed memory and ordinary file I/O can interact with the Linux page cache.

Conceptually:

```text
Disk
 |
 v
Page Cache
 |
 +---- read()
 |
 +---- mmap()
```

For a file-backed mapping:

```text
process accesses mapped file page
             |
             v
        page fault
             |
             v
       page cache lookup
             |
       +-----+-----+
       |           |
     found       missing
       |           |
       v           v
     map        read data
                  |
                  v
              cache/page
                  |
                  v
                map
```

This connects virtual memory with filesystem internals.

---

# 47. Anonymous Pages vs File-Backed Pages

## Anonymous

Examples:

```text
heap
stack
anonymous mmap
```

No ordinary filesystem file supplies their contents.

## File-backed

Examples:

```text
executable
shared library
mmap(file)
```

Contents can be associated with a file mapping and page cache.

---

# 48. Shared Memory Through `mmap()` [LSP]

Two processes can map the same physical pages.

Conceptually:

```text
Process A VA
     |
     v
Physical Page X
     ^
     |
Process B VA
```

This enables high-performance IPC.

However, shared memory requires synchronization:

```text
mutex
semaphore
futex
atomic operations
etc.
```

Otherwise it can produce race conditions.

---

# 49. `MAP_SHARED` vs `MAP_PRIVATE`

Important distinction.

```text
MAP_SHARED
```

Updates to the mapping can be visible through the shared mapping according to the mapping/file semantics.

```text
MAP_PRIVATE
```

uses private copy-on-write semantics for modifications.

Conceptually:

```text
MAP_PRIVATE:

A ----+
      |
      v
   Page X
      ^
      |
B ----+

B writes
   |
   v
COW
   |
   +--> B gets private page
```

---

# 50. `mprotect()` [LSP]

`mprotect()` changes memory protection for a mapped region.

Example:

```c
mprotect(addr, size, PROT_READ);
```

Possible protections include:

```text
PROT_READ
PROT_WRITE
PROT_EXEC
PROT_NONE
```

This demonstrates that protection is part of virtual memory management.

---

# 51. `madvise()` [LSP]

`madvise()` allows a process to provide usage advice about memory mappings.

Examples include advice concerning:

```text
sequential access
random access
page reuse
discarding pages
```

The kernel may use this information to optimize memory behavior.

It is advice, not a direct command to perform arbitrary physical-memory operations.

---

# 52. `mlock()` [LSP]

`mlock()` requests that pages remain resident in RAM rather than being swapped out under normal memory-management behavior.

Example:

```c
mlock(addr, length);
```

Important:

```text
mlock() affects residency
not virtual-address translation itself
```

It is subject to resource limits and privileges/policy.

---

# 53. Memory Overcommit

Linux can use virtual memory accounting/overcommit policies that allow processes to reserve more virtual memory than can be simultaneously backed by physical RAM.

This is useful because:

```text
virtual allocation
```

does not always mean:

```text
physical page immediately consumed
```

But excessive allocation can eventually create memory pressure and, depending on policy and circumstances, allocation failures or OOM behavior.

---

# 54. OOM

OOM means:

```text
Out Of Memory
```

When the system cannot satisfy memory requirements under its policies, Linux may invoke the OOM handling machinery.

The kernel may select processes for termination based on an OOM scoring mechanism.

Important:

```text
malloc() returning successfully
```

does not guarantee that the process can keep arbitrary amounts of memory resident forever.

---

# 55. Swap [OS/KERNEL]

Swap provides storage-backed space that can hold memory contents that are not currently resident in RAM.

Conceptually:

```text
RAM
 |
 | memory pressure
 v
Reclaim
 |
 v
Swap/storage
```

Later:

```text
access
 |
 v
page fault
 |
 v
read page back
 |
 v
RAM
```

Swap is not simply "extra RAM"; it is much slower than RAM.

---

# 56. Page Reclaim

When memory pressure occurs, Linux can reclaim memory.

Potential candidates include:

```text
clean file-backed pages
reclaimable cache
anonymous pages under appropriate conditions
```

Dirty data may need writeback before its page can be reclaimed.

This is one reason page cache and virtual memory are tightly connected.

---

# 57. Huge Pages

Normal pages may be:

```text
4 KiB
```

Huge-page mechanisms use larger page sizes.

Benefits can include:

```text
fewer page-table entries
larger TLB coverage
lower translation overhead
```

Trade-offs can include:

```text
internal fragmentation
allocation complexity
less flexibility
```

---

# 58. TLB [OS/KERNEL]

The CPU would otherwise need to walk page tables for many memory accesses.

The **Translation Lookaside Buffer (TLB)** caches recent virtual-to-physical translations.

Conceptually:

```text
Virtual Address
      |
      v
    TLB
   /   \
 hit    miss
 |       |
 v       v
PA    Page-table walk
          |
          v
        TLB fill
```

A TLB miss does not necessarily mean a page fault.

---

# 59. TLB Miss vs Page Fault

Very important interview distinction:

```text
TLB miss
```

means:

```text
translation not found in TLB
```

The page table may still contain a valid mapping.

Whereas:

```text
Page fault
```

means the memory access cannot be completed using the current translation/protection state and requires fault handling.

Therefore:

```text
TLB miss != page fault
```

---

# 60. Context Switch and Address Spaces

When switching between processes with different address spaces, the CPU must ensure translations from the previous address space are not incorrectly reused.

Architectures provide mechanisms such as:

```text
TLB invalidation
address-space identifiers
PCID/ASID-like facilities
```

The exact behavior is architecture-specific.

This is one reason address-space switching has performance implications.

---

# 61. Copy-on-Write and Page Tables

After `fork()`:

```text
Parent page table
       |
       +----> Page X

Child page table
       |
       +----> Page X
```

Both point to the same physical page.

The mappings are arranged so writes trigger COW handling.

After child writes:

```text
Parent page table -> Page X

Child page table  -> Page Y
```

This is an excellent example of how:

```text
processes
+
page tables
+
page faults
+
physical pages
```

work together.

---

# 62. Page Reference / Sharing Concept

A physical page can be referenced by multiple mappings.

For example:

```text
Process A
     |
     v
   Page X
     ^
     |
Process B
```

The kernel tracks page usage and ownership/reference relationships using its memory-management structures.

This is necessary for:

```text
COW
shared mappings
page reclamation
safe page lifetime
```

---

# 63. Virtual Memory and Security

Virtual memory supports:

```text
process isolation
read/write permissions
execute permissions
ASLR
guard regions
kernel/user separation
```

ASLR randomizes relevant memory locations to make certain memory-corruption attacks harder.

It is a security feature built around virtual address-space layout randomization.

---

# 64. ASLR

Without randomization:

```text
Stack -> predictable
Libraries -> predictable
Heap -> predictable
```

With ASLR:

```text
Stack -> randomized
Libraries -> randomized
Heap -> randomized
mmap regions -> randomized
```

The exact regions affected depend on architecture and configuration.

---

# 65. `exec()` and Virtual Memory [LSP]

When a process calls an `exec` family function successfully:

```text
old program image
       |
       v
replaced
       |
       v
new executable image
```

The process's memory mappings are replaced as part of loading the new program image.

This is why the common:

```text
fork()
exec()
```

pattern is efficient with COW.

---

# 66. `fork()` + `exec()` Full Picture

```text
Parent
  |
  | fork()
  v
Parent + Child
  |
  +--> initially share pages using COW
  |
  v
Child calls exec()
  |
  v
Child address space replaced
  |
  v
New executable loaded/mapped
```

Only pages actually modified before `exec()` require COW copying.

---

# 67. Virtual Memory and `read()`

Normal file read:

```text
read(fd, user_buffer, size)
```

conceptually:

```text
File
 |
 v
Kernel/file subsystem
 |
 v
page cache / storage
 |
 v
copy data into user buffer
```

Whereas `mmap()`:

```text
File
 |
 v
mapping
 |
 v
user virtual address
 |
 v
fault/page-cache path as required
 |
 v
CPU reads mapped memory
```

Both can ultimately interact with the page cache, but their userspace access models differ.

---

# 68. `mmap()` vs `read()`

## `read()`

```text
application buffer
       ^
       |
kernel copies data
       |
       ^
file/cache
```

## `mmap()`

```text
application virtual address
       |
       v
file-backed mapping
       |
       v
page-cache-backed pages as appropriate
```

`mmap()` can avoid explicit user-buffer copying for the application access pattern, but it is not automatically faster for every workload.

---

# 69. Virtual Memory and Kernel Internals Roadmap

The concepts connect like this:

```text
Process
   |
   v
mm_struct
   |
   v
VMA
   |
   v
Page Table
   |
   v
TLB/MMU
   |
   v
Physical Page
   |
   +--> Page Cache
   |
   +--> Anonymous Memory
   |
   +--> Swap
```

Fault path:

```text
CPU access
   |
   v
TLB/MMU
   |
   v
Page fault
   |
   v
Kernel MM
   |
   v
VMA + page-table state
   |
   v
Allocate / locate / copy / load
   |
   v
Update mapping
   |
   v
Resume execution
```

---

# 70. Practical Lab — Observe Virtual Memory

Run:

```bash
cat /proc/self/maps
```

This displays virtual memory mappings for the process.

You can also inspect a running process:

```bash
cat /proc/<pid>/maps
```

You will see mappings associated with:

```text
executable
shared libraries
heap
stack
mmap regions
```

---

# 71. `/proc/<pid>/maps`

Typical conceptual output:

```text
00400000-00401000 r--p ...
00401000-00402000 r-xp ...
00600000-00601000 rw-p ... [heap]
7f...-7f... r-xp ... libc.so
7fff...-7fff... rw-p ... [stack]
```

Fields include:

```text
address range
permissions
mapping type
file/offset information
```

Exact output varies.

---

# 72. `/proc/<pid>/status`

Useful memory-related fields can include:

```text
VmSize
VmRSS
VmData
VmStk
VmExe
VmLib
```

Conceptually:

```text
VmSize = virtual memory size
VmRSS  = resident set size
```

These are not interchangeable.

---

# 73. Virtual Size vs RSS

Very important:

```text
Virtual memory size
```

describes the process's virtual address-space usage/accounting.

```text
RSS
```

represents pages currently resident in physical memory for the process, subject to shared-page/accounting details.

Therefore:

```text
Virtual size can be much larger than RSS.
```

Example:

```text
Virtual = 2 GB
RSS     = 100 MB
```

can be perfectly possible.

---

# 74. Observe Page Faults

Linux process statistics can provide page-fault counters.

For example:

```bash
cat /proc/<pid>/stat
```

contains process accounting fields, including fault-related counters.

Tools such as:

```bash
/usr/bin/time -v ./program
```

can also report page-fault statistics on systems where the implementation provides them.

---

# 75. Simple Fault Experiment

Create a program that allocates a large anonymous region:

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    size_t size = 100 * 1024 * 1024;

    char *p = malloc(size);

    if (!p)
        return 1;

    printf("allocated virtual memory\n");

    memset(p, 1, size);

    printf("touched memory\n");

    free(p);

    return 0;
}
```

Conceptually compare:

```text
malloc()
```

with:

```text
memset()
```

The first actual touches of pages can trigger memory population/fault activity.

---

# 76. Why Touching Memory Matters

Consider:

```c
char *p = malloc(100 * 1024 * 1024);
```

versus:

```c
memset(p, 0, 100 * 1024 * 1024);
```

The first requests memory from the allocator.

The second actually accesses many pages.

Therefore the physical memory and page-fault behavior can differ significantly.

---

# 77. `calloc()` and Zero Pages

`calloc()` returns zero-initialized memory.

The implementation can use efficient zeroing mechanisms, and the kernel can optimize zero-filled memory using mechanisms such as shared zero pages where appropriate.

Do not assume that every byte requires an immediate unique physical page at allocation time.

---

# 78. Memory Mapping Lifecycle

A useful mental model:

```text
Virtual address range requested
          |
          v
VMA/mapping established
          |
          v
No physical page required immediately in all cases
          |
          v
Process accesses page
          |
          v
Page fault
          |
          v
Kernel resolves fault
          |
          v
Page table updated
          |
          v
Instruction retries
          |
          v
Execution continues
```

---

# 79. Page Fault and Instruction Retry

A recoverable page fault generally works conceptually like:

```text
Instruction
    |
    v
Memory access
    |
    v
Fault
    |
    v
Kernel fixes mapping
    |
    v
Return from fault handler
    |
    v
Instruction executes again
```

The CPU architecture provides the mechanism to resume the faulting execution appropriately.

---

# 80. Major Interview Trap: `malloc()` vs Physical RAM

Wrong:

> `malloc()` always allocates physical RAM immediately.

Better:

> `malloc()` is a userspace allocator. It obtains/uses virtual address space through mechanisms such as `brk()` and `mmap()`, and physical page population can be lazy depending on the memory and allocator behavior.

---

# 81. Major Interview Trap: TLB Miss vs Page Fault

Wrong:

> TLB miss means page fault.

Correct:

```text
TLB miss
    |
    v
page-table walk
    |
    +--> valid mapping -> continue

    +--> invalid/not usable -> page fault
```

Therefore a TLB miss can be handled without a page fault.

---

# 82. Major Interview Trap: Virtual Memory = Swap

Wrong:

> Virtual memory means swap.

Correct:

> Virtual memory is the broader address-space and memory-management abstraction. Swap is only one mechanism used by the OS to manage some memory contents under memory pressure.

---

# 83. Major Interview Trap: VMA = Physical Memory

Wrong:

> A VMA represents physical pages.

Correct:

> A VMA describes properties of a virtual address range. Physical-page mappings are represented through page tables and associated memory-management structures.

---

# 84. Major Interview Trap: Page Fault = Segmentation Fault

Wrong:

> Every page fault causes SIGSEGV.

Correct:

```text
Page fault
    |
    +--> valid/recoverable
    |      |
    |      +--> kernel resolves it
    |
    +--> invalid/protection violation
           |
           +--> may result in SIGSEGV/SIGBUS depending on cause
```

---

# 85. Senior Interview Question: Explain `fork()` with COW

Strong answer:

> `fork()` creates a child with a logically separate address space. Linux can initially share physical pages between parent and child using copy-on-write. The relevant mappings are protected so that a write triggers a page fault. The kernel then creates a private copy for the writing process and updates its page-table mapping. This avoids eagerly copying the entire address space.

---

# 86. Senior Interview Question: What Happens on a Page Fault?

Strong high-level answer:

```text
CPU detects translation/protection fault
        |
        v
architecture-specific fault entry
        |
        v
kernel identifies faulting address/access
        |
        v
finds applicable VMA
        |
        v
determines cause
        |
        +--> anonymous page
        +--> file-backed page
        +--> COW
        +--> swap
        +--> invalid access
        |
        v
resolves or rejects access
        |
        v
resume instruction or signal process
```

---

# 87. Senior Interview Question: What Is the Difference Between VMA and Page Table?

Answer:

```text
VMA:
describes a virtual address region and its properties.

Page table:
provides the hardware translation/protection state
for virtual pages.
```

Example:

```text
VMA:
100 MB region, RW

Page tables:
page 0 -> physical frame 10
page 1 -> physical frame 55
page 2 -> not currently mapped
...
```

---

# 88. Senior Interview Question: Why Multi-Level Page Tables?

Answer:

> A flat page table for a large virtual address space can consume significant memory even when only a small portion of the address space is used. Multi-level page tables allow lower-level structures to be allocated only for populated portions of the address space, reducing overhead.

---

# 89. Senior Interview Question: Why Does `fork()` Not Copy Everything?

Answer:

```text
Because of copy-on-write.

fork()
  |
  v
share pages
  |
  v
protect for COW
  |
  v
copy only pages that are written
```

This makes process creation much cheaper than eager copying.

---

# 90. Senior Interview Question: What Is a Major Page Fault?

Answer:

> A major fault generally means resolving the fault requires I/O, such as bringing required data from storage. A minor fault can be resolved without such storage I/O.

---

# 91. Senior Interview Question: Why Is Page Cache Related to `mmap()`?

Answer:

> File-backed mappings can use the filesystem/page-cache path. When a process accesses a file-backed mapped page that is not currently resident, the page-fault path can locate or bring the file data into memory and establish the required mapping.

---

# 92. Senior Interview Question: Why Use Huge Pages?

Answer:

```text
larger page size
      |
      +--> fewer page-table entries
      |
      +--> larger TLB coverage
      |
      +--> potentially fewer TLB misses
```

But:

```text
larger allocation granularity
      |
      +--> possible fragmentation/trade-offs
```

---

# 93. Senior Interview Question: What Is Memory Overcommit?

Answer:

> Memory overcommit allows the system to account for virtual memory commitments in a way that can exceed immediately available physical RAM under configured policies. This works because virtual address-space reservation and immediate physical-page residency are not the same thing. Under pressure, allocations or accesses can eventually fail or trigger OOM behavior depending on policy and available resources.

---

# 94. Senior Interview Question: What Is ASLR?

Answer:

> Address Space Layout Randomization changes the locations of important memory regions between executions to make memory-corruption exploitation harder by reducing predictability of addresses.

---

# 95. Senior Interview Question: How Does `mmap()` Connect to the Kernel?

High-level:

```text
User:
mmap()

   |
   v

System call entry

   |
   v

Linux VM subsystem

   |
   v

virtual memory area established

   |
   v

page-table mappings established/created as needed

   |
   v

later memory access

   |
   v

page fault if required

   |
   v

fault handler resolves mapping
```

---

# 96. OS + LSP + Kernel Integration

The complete picture:

```text
                  PROCESS
                     |
                     v
             Virtual Address Space
                     |
        +------------+-------------+
        |            |             |
       Heap        Stack        mmap/File
        |            |             |
        +------------+-------------+
                     |
                     v
                 VMA layer
                     |
                     v
                Page Tables
                     |
                     v
                  TLB/MMU
                     |
                     v
               Physical Pages
                     |
          +----------+----------+
          |                     |
      Anonymous              Page Cache
          |                     |
          |                     v
          |                   Files
          |
         Swap
```

This is the core bridge between OS theory, Linux programming and kernel internals.

---

# 97. Chapter 7 Cheat Sheet

```text
VIRTUAL MEMORY
    |
    +-- virtual address
    +-- physical address
    +-- pages
    +-- physical frames
    +-- page tables
    +-- TLB
    +-- page faults
    +-- protection
    +-- process isolation

LINUX
    |
    +-- mmap()
    +-- munmap()
    +-- mprotect()
    +-- madvise()
    +-- mlock()
    +-- malloc()
    +-- brk()/sbrk()
    +-- fork()
    +-- exec()

KERNEL
    |
    +-- mm_struct
    +-- VMA
    +-- page tables
    +-- fault handling
    +-- anonymous memory
    +-- page cache
    +-- reclaim
    +-- swap
    +-- COW

IMPORTANT DISTINCTIONS
    |
    +-- virtual memory != swap
    +-- TLB miss != page fault
    +-- VMA != physical page
    +-- page fault != SIGSEGV
    +-- malloc() != system call
```

---

# 98. Final Key Takeaways

1. Virtual memory gives each process an isolated virtual address space.
2. Virtual addresses are translated to physical addresses using page tables and hardware MMU mechanisms.
3. Memory is managed in page-sized units.
4. Physical RAM is managed in page frames.
5. Multi-level page tables reduce page-table memory overhead for sparse address spaces.
6. The TLB caches recent virtual-to-physical translations.
7. A TLB miss is not automatically a page fault.
8. A page fault can be a normal and recoverable event.
9. Demand paging allows physical pages to be populated when required.
10. Copy-on-write makes `fork()` efficient.
11. A write to a COW page can trigger a page fault and private-page creation.
12. `mmap()` establishes virtual memory mappings.
13. `malloc()` is a library allocator, not a system call.
14. `malloc()` can use mechanisms such as `brk()` and `mmap()`.
15. Virtual allocation and physical residency are different concepts.
16. Anonymous memory is used for regions such as heap and stack.
17. File-backed mappings connect virtual memory with the filesystem and page cache.
18. `MAP_PRIVATE` uses private COW semantics for modifications.
19. `MAP_SHARED` enables shared mapping semantics.
20. `mprotect()` changes memory protection.
21. `mlock()` concerns page residency, not address translation.
22. Linux uses `mm_struct` to represent a process memory context.
23. VMAs describe virtual address ranges.
24. Page tables describe translation/protection state.
25. Page-cache pages can be used by file-backed memory mappings.
26. Swap is a memory-management mechanism, not the definition of virtual memory.
27. Memory reclaim happens under memory pressure.
28. Huge pages can improve TLB reach but introduce trade-offs.
29. ASLR randomizes address-space layout for security.
30. `/proc/<pid>/maps` is a useful way to inspect process mappings.
31. RSS and virtual memory size measure different things.
32. The page-fault path is a major connection between user-space execution and kernel memory management.
33. Understanding virtual memory is essential before studying page tables, TLBs and page faults in detail.

---

# Chapter 8 Preview — Page Tables + TLB + Page Faults

Next chapter will go deeper into the translation path:

```text
Virtual Address
       |
       v
+----------------+
|      TLB       |
+----------------+
       |
   hit | miss
       |
       v
Page-table walk
       |
       v
PTE / permissions
       |
       +---- valid ----> Physical Frame
       |
       +---- invalid --> Page Fault
                              |
                              v
                       Linux MM subsystem
                              |
                 +------------+------------+
                 |            |            |
               COW       File-backed    Anonymous
                 |            |            |
                 +------------+------------+
                              |
                              v
                       update page table
                              |
                              v
                       resume execution
```

Chapter 8 will focus specifically on **page-table levels, PTEs, TLB behavior, TLB misses, page-fault types, COW faults, page-table walks, and the Linux kernel fault path**.
