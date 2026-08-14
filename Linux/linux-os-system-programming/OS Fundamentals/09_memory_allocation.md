# Chapter 9 — Memory Allocation

> **Three-layer approach**
>
> This chapter connects:
> 1. **[OS] Operating System memory-allocation concepts**
> 2. **[LSP] Linux System Programming + C code**
> 3. **[KERNEL] Linux Kernel memory-allocation internals**
>
> The goal is to understand the complete path:
>
> ```text
> Application
>     ↓
> malloc()
>     ↓
> userspace allocator
>     ↓
> brk()/mmap()
>     ↓
> Virtual Memory
>     ↓
> Page Fault
>     ↓
> Linux Memory Management
>     ↓
> Physical Pages
> ```

---

# 1. What Is Memory Allocation?

Memory allocation means obtaining a region of memory for use by a program or kernel component.

There are two major environments:

```text
User space
    |
    +--> malloc()
    +--> calloc()
    +--> realloc()
    +--> mmap()

Kernel space
    |
    +--> kmalloc()
    +--> kzalloc()
    +--> alloc_pages()
    +--> vmalloc()
```

These APIs operate at different layers.

---

# 2. Process Memory Layout

A simplified Linux process address space looks like:

```text
High addresses
+----------------------+
|        Stack         |
|          ↓           |
+----------------------+
|                      |
|    mmap region       |
| shared libraries     |
|                      |
+----------------------+
|          ↑           |
|        Heap          |
+----------------------+
| .bss                 |
| .data                |
| .rodata              |
| .text                |
+----------------------+
Low addresses
```

This is conceptual. Exact layout depends on architecture, ASLR, executable format and kernel configuration.

---

# 3. Text, Data, BSS, Heap and Stack

## Text

Contains executable program code.

Typically:

```text
read + execute
```

## Data

Contains initialized global/static variables.

Example:

```c
int global = 10;
```

## BSS

Contains zero-initialized/uninitialized global/static storage.

Example:

```c
int global;
static int counter;
```

## Heap

Used for dynamic allocation.

Example:

```c
int *p = malloc(sizeof(int));
```

## Stack

Used for automatic function-call state and local variables.

Example:

```c
void f(void)
{
    int x = 10;
}
```

---

# 4. Stack vs Heap

| Feature | Stack | Heap |
|---|---|---|
| Typical use | Function-local data | Dynamic allocation |
| Lifetime | Scope/call dependent | Explicit allocation/deallocation |
| Management | Compiler/runtime conventions | Allocator |
| Typical API | Automatic | `malloc/free` |
| Growth | Usually automatic | Allocator-controlled |
| Fragmentation | Usually not allocator fragmentation | Possible |
| Common bug | Stack overflow | Leak/use-after-free |

Important:

> Stack and heap are both part of a process's virtual address space.

---

# 5. Static Storage vs Dynamic Storage

Example:

```c
int global = 10;

int main(void)
{
    static int x = 20;

    int *p = malloc(sizeof(int));

    free(p);
}
```

Conceptually:

```text
global/x
   ↓
static storage

p
 ↓
stack variable containing address

allocated object
 ↓
heap/dynamic storage
```

---

# 6. Automatic vs Dynamic Allocation

Automatic:

```c
void f(void)
{
    int x = 10;
}
```

The compiler and calling convention arrange storage for `x`.

Dynamic:

```c
int *p = malloc(sizeof(*p));
```

The program explicitly requests dynamic storage.

It must eventually release it:

```c
free(p);
```

---

# 7. `malloc()`

Prototype:

```c
void *malloc(size_t size);
```

Example:

```c
int *p = malloc(sizeof(*p));

if (p == NULL)
{
    perror("malloc");
    return 1;
}

*p = 100;

free(p);
```

Important properties:

```text
malloc()
    |
    +--> returns suitably aligned storage
    |
    +--> contents are uninitialized
    |
    +--> returns NULL on failure
```

Do not read from allocated memory before initializing it.

---

# 8. `calloc()`

Prototype:

```c
void *calloc(size_t nmemb, size_t size);
```

Example:

```c
int *p = calloc(10, sizeof(*p));

if (!p)
    return 1;

for (int i = 0; i < 10; ++i)
    printf("%d\n", p[i]);

free(p);
```

`calloc()` allocates storage for an array and initializes the returned bytes to zero.

---

# 9. `realloc()`

Prototype:

```c
void *realloc(void *ptr, size_t size);
```

Example:

```c
int *p = malloc(10 * sizeof(*p));

if (!p)
    return 1;

int *tmp = realloc(p, 20 * sizeof(*p));

if (!tmp)
{
    free(p);
    return 1;
}

p = tmp;

free(p);
```

Important:

```text
realloc()
    |
    +--> may keep same address
    |
    +--> may move allocation
    |
    +--> preserves old contents up to the relevant size
    |
    +--> new memory, if any, is not initialized
```

Always use a temporary pointer when failure must preserve the original allocation.

---

# 10. `free()`

Example:

```c
int *p = malloc(sizeof(*p));

if (!p)
    return 1;

*p = 42;

free(p);
p = NULL;
```

After:

```c
free(p);
```

the allocated object is no longer valid to access.

The pointer variable itself still exists, but its old value must not be dereferenced.

---

# 11. Common Dynamic-Memory Bugs

## Memory leak

```c
void f(void)
{
    int *p = malloc(100);

    if (!p)
        return;

    /* forgot free(p) */
}
```

The allocation becomes unreachable.

---

## Use-after-free

```c
int *p = malloc(sizeof(*p));

free(p);

*p = 10;       /* BUG */
```

---

## Double free

```c
free(p);
free(p);       /* BUG */
```

---

## Buffer overflow

```c
int *p = malloc(2 * sizeof(*p));

p[2] = 10;     /* BUG */
```

Valid indexes are:

```text
0
1
```

---

# 12. `malloc()` Does Not Mean "Get Physical RAM Immediately"

This is extremely important.

Consider:

```c
char *p = malloc(100 * 1024 * 1024);
```

The application receives virtual address space from the allocator.

It does not mean:

```text
100 MB of physical RAM has immediately been populated
```

A simplified path is:

```text
malloc()
   ↓
allocator obtains/uses virtual address space
   ↓
application receives address
   ↓
first access to a page
   ↓
page fault if not populated
   ↓
kernel provides physical page
```

This connects Chapter 8 with Chapter 9.

---

# 13. Userspace Allocator

`malloc()` is normally implemented by a userspace memory allocator rather than directly being a system call.

Conceptually:

```text
Application
    |
    v
malloc()
    |
    v
C library allocator
    |
    +--> reuse existing free chunks
    |
    +--> request more virtual memory
             |
             +--> brk()
             |
             +--> mmap()
```

Exact behavior depends on the allocator/libc implementation and allocation size.

---

# 14. `malloc()` Is Not a System Call

This is a common interview question.

```text
malloc()
```

is a library/allocator API.

It may internally use system calls such as:

```text
brk()
mmap()
munmap()
```

Therefore:

```text
malloc != system call
```

---

# 15. `brk()` and `sbrk()`

Historically, the process heap can be extended using the program break.

Conceptually:

```text
Heap
  ↑
program break
```

Increasing the break:

```text
old break
    |
    v
new break
```

extends the process's heap-related virtual address range.

Linux also provides:

```c
brk()
sbrk()
```

but application code should generally use `malloc()` rather than manipulating the program break directly.

---

# 16. `sbrk()` Example

```c
#include <unistd.h>
#include <stdio.h>

int main(void)
{
    void *old = sbrk(0);

    printf("program break: %p\n", old);

    void *result = sbrk(4096);

    if (result == (void *)-1)
    {
        perror("sbrk");
        return 1;
    }

    printf("old break returned by sbrk: %p\n", result);
    printf("new break: %p\n", sbrk(0));

    return 0;
}
```

For learning purposes, this demonstrates the program-break mechanism.

Do not build a general-purpose allocator around direct `sbrk()` manipulation in normal application code.

---

# 17. `mmap()` for Anonymous Memory

Example:

```c
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    size_t size = 4096;

    void *p = mmap(
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

    ((char *)p)[0] = 'A';

    printf("%c\n", ((char *)p)[0]);

    munmap(p, size);

    return 0;
}
```

---

# 18. `mmap()` vs `malloc()`

| `malloc()` | `mmap()` |
|---|---|
| Library allocator API | System call |
| Manages allocation internally | Direct mapping request |
| Good general-purpose API | Useful for explicit mappings |
| Can use `brk`/`mmap` internally | Directly creates mapping |
| `free()` releases allocation | `munmap()` removes mapping |

Do not assume every `malloc()` call becomes exactly one `mmap()` call.

---

# 19. Why Allocators Reuse Memory

Suppose:

```c
p = malloc(100);
free(p);
q = malloc(100);
```

The allocator may reuse the previously freed chunk.

Conceptually:

```text
malloc(100)
   ↓
chunk A

free(chunk A)
   ↓
free list/cache

malloc(100)
   ↓
reuse chunk A
```

This avoids unnecessary kernel interactions.

---

# 20. Allocation Granularity

The allocator manages memory in chunks.

Requested:

```text
malloc(13)
```

does not necessarily result in exactly 13 bytes of internal storage.

There can be:

```text
alignment
metadata
size classes
padding
free-space management
```

Therefore actual allocator consumption can be greater than the requested size.

---

# 21. Alignment

Many CPU types require or benefit from aligned accesses.

`malloc()` returns memory suitably aligned for the types supported by the standard C allocation interface.

Example:

```c
struct Data {
    long x;
    double y;
};

struct Data *p = malloc(sizeof(*p));
```

The returned pointer is suitably aligned for the object type.

---

# 22. Internal Fragmentation

Internal fragmentation occurs when allocated storage is larger than the requested payload.

Example:

```text
Requested: 13 bytes

Allocator chunk:
+----------------------+
| metadata/padding     |
| user area            |
+----------------------+
```

The difference can contribute to internal overhead.

---

# 23. External Fragmentation

External fragmentation occurs when free memory exists but is split into pieces that are not useful for a requested allocation.

Conceptually:

```text
Used | Free | Used | Free | Used | Free
```

Total free memory may be sufficient, but a large contiguous region may not be available for an allocation strategy that requires it.

---

# 24. Fragmentation Comparison

```text
Internal:
allocation itself contains unused space.

External:
free space exists but is fragmented.
```

Interview shortcut:

```text
Internal = waste inside allocated blocks
External = free space scattered outside allocated blocks
```

---

# 25. Memory Pool

A memory pool preallocates a region and serves fixed-size objects from it.

Conceptually:

```text
Pool
+----+----+----+----+----+
| B1 | B2 | B3 | B4 | B5 |
+----+----+----+----+----+
```

Allocation:

```text
get free block
```

Deallocation:

```text
return block to pool
```

Benefits:

```text
fast allocation
predictable behavior
less fragmentation for fixed-size objects
```

---

# 26. Simple C Memory Pool

```c
#include <stdio.h>
#include <stddef.h>

#define BLOCK_SIZE 64
#define BLOCK_COUNT 10

static unsigned char pool[BLOCK_SIZE * BLOCK_COUNT];
static int used[BLOCK_COUNT];

void *pool_alloc(void)
{
    for (int i = 0; i < BLOCK_COUNT; ++i)
    {
        if (!used[i])
        {
            used[i] = 1;
            return &pool[i * BLOCK_SIZE];
        }
    }

    return NULL;
}

void pool_free(void *ptr)
{
    if (!ptr)
        return;

    unsigned char *p = ptr;

    if (p < pool ||
        p >= pool + sizeof(pool))
        return;

    size_t offset = (size_t)(p - pool);

    if (offset % BLOCK_SIZE != 0)
        return;

    int index = offset / BLOCK_SIZE;

    if (index >= 0 && index < BLOCK_COUNT)
        used[index] = 0;
}

int main(void)
{
    void *a = pool_alloc();
    void *b = pool_alloc();

    printf("a = %p\n", a);
    printf("b = %p\n", b);

    pool_free(a);
    pool_free(b);

    return 0;
}
```

This is a teaching example, not a production allocator.

---

# 27. Why Memory Pools Are Useful in Embedded Systems

Embedded systems often care about:

```text
predictable latency
limited RAM
fragmentation
deterministic behavior
```

Repeated:

```text
malloc()
free()
```

can introduce fragmentation and unpredictable allocation latency.

A pool can provide:

```text
O(1) or predictable allocation
fixed-size blocks
controlled memory usage
```

depending on the implementation.

---

# 28. Kernel Memory Allocation

Kernel code cannot simply use normal user-space:

```c
malloc()
```

Instead, Linux provides kernel allocation mechanisms.

Important APIs:

```text
kmalloc()
kzalloc()
kcalloc()
alloc_pages()
vmalloc()
```

---

# 29. `kmalloc()`

Typical use:

```c
void *p = kmalloc(size, GFP_KERNEL);
```

Conceptually:

```text
kmalloc()
   |
   v
kernel allocator
   |
   v
physically contiguous memory
```

For normal `kmalloc()` allocations, the memory is physically contiguous within the allocation, subject to the allocator and size/order constraints.

---

# 30. `kzalloc()`

```c
void *p = kzalloc(size, GFP_KERNEL);
```

It allocates kernel memory and zero-initializes it.

Equivalent conceptual intent:

```text
kmalloc()
+
zero initialization
```

Use it when zeroed kernel memory is required.

---

# 31. `kcalloc()`

For array-like allocations:

```c
void *p = kcalloc(n, size, GFP_KERNEL);
```

It allocates space for multiple elements and zero-initializes the memory.

---

# 32. `GFP_KERNEL`

Example:

```c
kmalloc(1024, GFP_KERNEL);
```

The GFP flags tell the kernel allocator about the allocation context and allowed behavior.

`GFP_KERNEL` is commonly used in normal process context where the caller may sleep.

This is important:

> Allocation flags are not merely optimization hints; they describe what the allocator is allowed to do in the current execution context.

---

# 33. Why Allocation Context Matters

Suppose code executes in a context where sleeping is not allowed.

The allocator must not perform operations that can sleep.

Therefore:

```text
Process context
    |
    +--> may be able to sleep
    |
    +--> GFP_KERNEL often appropriate

Atomic/interrupt-like context
    |
    +--> cannot sleep
    |
    +--> use an appropriate non-sleeping allocation mode
```

The exact flag depends on the context.

---

# 34. `kmalloc()` and Physical Contiguity

A key interview distinction:

```text
kmalloc()
```

provides memory that is physically contiguous for the allocation.

This makes it useful when a device or subsystem needs a physically contiguous buffer, subject to the allocation size and constraints.

---

# 35. `vmalloc()`

`vmalloc()` provides virtually contiguous kernel memory.

Conceptually:

```text
Virtual:
+----+----+----+----+
| V1 | V2 | V3 | V4 |
+----+----+----+----+
  |    |    |    |
  v    v    v    v
Physical:
P10  P87  P21  P42
```

Virtual addresses are contiguous.

Physical pages do not have to be contiguous.

---

# 36. `kmalloc()` vs `vmalloc()`

```text
kmalloc():
Virtual contiguous
+
Physical contiguous
```

```text
vmalloc():
Virtual contiguous
+
Physical pages may be non-contiguous
```

This is one of the most frequently asked Linux kernel interview questions.

---

# 37. Why Not Always Use `vmalloc()`?

`vmalloc()` has overhead associated with establishing/managing virtual mappings.

Also, some hardware requires physically contiguous memory or DMA-specific memory handling.

Therefore:

```text
kmalloc()
```

and:

```text
vmalloc()
```

serve different purposes.

---

# 38. `alloc_pages()`

Linux also has a page-level allocator.

Conceptually:

```c
struct page *page = alloc_pages(gfp_mask, order);
```

The `order` determines the number of pages:

```text
number of pages = 2^order
```

Examples:

```text
order 0 -> 1 page
order 1 -> 2 pages
order 2 -> 4 pages
order 3 -> 8 pages
```

This is closely related to the buddy allocator.

---

# 39. Buddy Allocator

The buddy allocator manages physical memory in power-of-two blocks.

Conceptually:

```text
Large block
    |
    +---- split ----+
    |               |
 buddy A          buddy B
    |
    +---- split ----+
```

If a requested order is unavailable:

```text
larger block
    ↓
split
    ↓
smaller buddies
```

When buddies become free, they can potentially be merged.

---

# 40. Why Is It Called Buddy?

Suppose:

```text
Block A
Block B
```

are adjacent blocks of the same size produced by splitting a larger block.

They are buddies.

If both become free:

```text
A free + B free
       |
       v
merge into larger block
```

This allows the allocator to reconstruct larger blocks.

---

# 41. Buddy Allocator and Fragmentation

The buddy system helps manage physical memory efficiently using power-of-two blocks.

Advantages:

```text
fast splitting
fast merging
simple structure
supports page-order allocations
```

Potential issue:

```text
internal fragmentation
```

because a request may require a power-of-two-sized block.

---

# 42. SLAB / SLUB

Allocating individual kernel objects directly from pages can be expensive.

Linux therefore uses object allocators such as:

```text
SLAB
SLUB
```

Modern Linux commonly uses SLUB.

Conceptually:

```text
Physical pages
      |
      v
SLUB cache
      |
      v
fixed-size kernel objects
```

Example:

```text
struct inode
struct task_struct
other frequently allocated objects
```

can be served efficiently through caches.

---

# 43. Why Object Caches?

Suppose the kernel repeatedly allocates:

```text
struct object
```

of the same size.

Instead of repeatedly building allocation structures from scratch:

```text
page allocation
object allocation
free
repeat
```

the slab/slub layer maintains caches of objects.

Benefits:

```text
faster allocation
object reuse
reduced fragmentation
constructor support in relevant contexts
better cache locality
```

---

# 44. Memory Allocation Layers in Linux

A useful conceptual hierarchy:

```text
Kernel subsystem
      |
      v
kmalloc()
      |
      v
SLUB / object allocator
      |
      v
page allocator
      |
      v
Buddy allocator
      |
      v
Physical pages
```

For `vmalloc()`:

```text
Kernel subsystem
      |
      v
vmalloc()
      |
      v
virtual address management
      |
      v
page allocation
      |
      v
non-contiguous physical pages
```

The exact internal path can vary.

---

# 45. User-Space vs Kernel-Space Allocation

| User space | Kernel space |
|---|---|
| `malloc()` | `kmalloc()` |
| `calloc()` | `kzalloc()` / `kcalloc()` |
| `mmap()` | `vmalloc()` / page allocation mechanisms |
| `free()` | `kfree()` |
| libc allocator | SLUB + page allocator |
| process virtual address space | kernel virtual address space |

---

# 46. `kfree()`

Kernel memory allocated by:

```c
kmalloc()
```

is normally released using:

```c
kfree(p);
```

Example:

```c
void *p = kmalloc(1024, GFP_KERNEL);

if (!p)
    return -ENOMEM;

/* use p */

kfree(p);
```

For memory from `vmalloc()`:

```c
vfree(p);
```

The allocation and deallocation APIs must match.

---

# 47. Matching Allocation and Free

Think:

```text
malloc()   -> free()
kmalloc()  -> kfree()
vmalloc()  -> vfree()
mmap()     -> munmap()
```

Do not mix them.

Bad:

```c
void *p = kmalloc(100, GFP_KERNEL);
free(p);                    /* wrong */
```

Correct:

```c
kfree(p);
```

---

# 48. Kernel Memory Flow

A simplified kernel allocation path:

```text
Kernel code
    |
    v
kmalloc(size, flags)
    |
    v
SLUB
    |
    +--> existing object available
    |        |
    |        v
    |      return
    |
    +--> need more backing pages
             |
             v
        page allocator
             |
             v
          buddy
             |
             v
        physical pages
```

---

# 49. `vmalloc()` Flow

```text
Kernel code
    |
    v
vmalloc(size)
    |
    v
kernel virtual address range
    |
    v
allocate physical pages
    |
    v
create mappings
    |
    v
virtually contiguous memory
```

Physical pages can be scattered.

---

# 50. Physical vs Virtual Contiguity

This distinction is fundamental.

### Physically contiguous

```text
Physical:

P100
P101
P102
P103
```

### Virtually contiguous

```text
Virtual:

V100
V101
V102
V103

Physical:

P100
P500
P210
P900
```

The virtual addresses remain contiguous even though physical frames are not.

---

# 51. Why DMA Changes the Picture

A CPU can use virtual mappings.

A device may require:

```text
DMA-capable physical addresses
```

or an IOMMU mapping.

Therefore, kernel code should not assume that:

```text
virtually contiguous == physically contiguous
```

For DMA, Linux provides dedicated DMA APIs such as:

```text
dma_alloc_coherent()
dma_map_single()
dma_map_sg()
```

depending on the use case.

---

# 52. `virt_to_phys()` Warning

Do not casually assume every kernel virtual address can be converted with:

```c
virt_to_phys()
```

The validity and meaning depend on the type of kernel address.

For example:

```text
kmalloc memory
vmalloc memory
user virtual address
```

are not interchangeable.

This is a common kernel interview trap.

---

# 53. User Virtual Address vs Kernel Virtual Address

User process:

```text
user virtual address
        |
        v
process page tables
```

Kernel:

```text
kernel virtual address
        |
        v
kernel address mappings
```

The kernel also accesses physical memory through virtual mappings.

---

# 54. Memory Zones

Linux physical memory management divides memory into zones to handle architectural and allocation constraints.

Common zone names include:

```text
ZONE_DMA
ZONE_DMA32
ZONE_NORMAL
ZONE_MOVABLE
```

Exact availability depends on architecture and configuration.

The important idea is:

> Different physical-memory regions can have different allocation constraints.

---

# 55. GFP Flags and Zones

Allocation requests include constraints.

Conceptually:

```text
Allocation request
      |
      +--> size
      +--> context
      +--> reclaim/sleep rules
      +--> memory constraints
```

The allocator uses the GFP flags and system state to determine what it can do.

---

# 56. Out Of Memory — OOM

If the system cannot satisfy memory demand after appropriate reclaim and other mechanisms, Linux can invoke OOM handling.

Conceptually:

```text
Allocation request
      |
      v
Can memory be reclaimed?
      |
   +--+--+
   |     |
  yes    no
   |     |
reclaim  |
   |     v
   +--> OOM handling
```

OOM behavior depends on the context and system configuration.

---

# 57. OOM Killer

In a memory-pressure situation, Linux may select a process for termination.

Conceptually:

```text
Severe memory pressure
        |
        v
OOM handling
        |
        v
select victim
        |
        v
terminate process
        |
        v
reclaim its memory
```

This is why a system can kill a process even though the application itself did not explicitly call `kill()`.

---

# 58. `malloc()` Failure vs OOM Killer

Do not assume:

```text
malloc() returns NULL
```

means the OOM killer must have killed a process.

Memory allocation behavior depends on:

```text
overcommit configuration
resource limits
address-space limits
available memory
allocator behavior
allocation type
```

A large virtual allocation may succeed initially and fail or fault later depending on circumstances.

---

# 59. Overcommit

Linux can allow committed virtual memory to exceed currently available physical RAM under configured policies.

Conceptually:

```text
Virtual commitments
        >
physical RAM
```

This is possible because not every allocated virtual page is necessarily touched simultaneously.

The important lesson:

```text
virtual allocation != immediate physical commitment
```

---

# 60. `RLIMIT_AS`

A process can have resource limits.

For example:

```bash
ulimit -v
```

relates to virtual-memory address-space limits in the shell/environment.

Applications can also inspect limits through:

```text
getrlimit()
```

This can affect whether allocations/mappings succeed.

---

# 61. Memory Mapping and Lazy Allocation

Consider:

```c
char *p = mmap(
    NULL,
    1024 * 1024 * 1024,
    PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_ANONYMOUS,
    -1,
    0
);
```

The virtual mapping may be established without immediately backing every page with a physical frame.

Then:

```c
p[0] = 1;
```

may fault in/populate one page.

Later:

```c
p[4096] = 2;
```

may populate another page.

This is demand paging.

---

# 62. Why `malloc()` Can Be Fast

For many allocations:

```text
malloc()
   |
   v
userspace allocator
   |
   v
reuse existing free chunk
```

No kernel transition is required for every allocation.

This is one reason general-purpose allocators maintain:

```text
free lists
caches
arenas
size classes
```

depending on the allocator implementation.

---

# 63. Allocator Metadata

A dynamic allocator needs bookkeeping.

Conceptually:

```text
+------------------+
| metadata         |
+------------------+
| user allocation  |
+------------------+
```

Metadata may contain information such as:

```text
size
allocation state
links to free structures
alignment information
```

Exact metadata layout is allocator-specific.

Do not rely on private allocator metadata in application code.

---

# 64. Why Corruption Is Dangerous

Suppose a buffer overflow overwrites allocator metadata.

```text
buffer
   |
   | overflow
   v
allocator metadata
```

Then a later:

```text
free()
malloc()
```

can detect or encounter corrupted state.

This can lead to:

```text
crash
memory corruption
security vulnerabilities
```

Modern allocators include various hardening mechanisms, but memory safety bugs remain dangerous.

---

# 65. Memory Leak Example

```c
#include <stdlib.h>

void bad(void)
{
    int *p = malloc(100 * sizeof(*p));

    if (!p)
        return;

    /* work */

    return;     /* leak */
}
```

Correct:

```c
void good(void)
{
    int *p = malloc(100 * sizeof(*p));

    if (!p)
        return;

    /* work */

    free(p);
}
```

---

# 66. Leak Detection with AddressSanitizer

Compile:

```bash
gcc -g -fsanitize=address -fno-omit-frame-pointer program.c -o program
```

Run:

```bash
./program
```

AddressSanitizer can detect many memory errors, including:

```text
heap-buffer-overflow
use-after-free
double-free
some leaks
```

Support and exact diagnostics depend on compiler/runtime configuration.

---

# 67. Valgrind

A common dynamic-memory debugging tool:

```bash
valgrind --leak-check=full ./program
```

It can help identify:

```text
memory leaks
invalid reads
invalid writes
use-after-free
```

It is generally much slower than normal execution.

---

# 68. `/proc` Memory Inspection

Useful:

```bash
cat /proc/<pid>/maps
```

and:

```bash
cat /proc/<pid>/status
```

You can inspect:

```text
virtual mappings
heap
stack
libraries
VmSize
VmRSS
```

For deeper information:

```bash
cat /proc/<pid>/smaps
```

`smaps` provides more detailed mapping-level memory information.

---

# 69. `pmap`

A useful command:

```bash
pmap <pid>
```

Example:

```bash
pmap -x <pid>
```

It helps summarize a process's virtual memory mappings.

---

# 70. `malloc_stats()` / Allocator-Specific Diagnostics

Some libc/allocator implementations provide allocator-specific diagnostic APIs or environment settings.

The important interview point is:

> `malloc()` is an interface; allocator internals are implementation-specific.

Therefore, do not assume a specific chunk layout or arena implementation unless discussing a known allocator version/configuration.

---

# 71. Memory Allocation and Page Faults

Connect Chapters 8 and 9:

```text
malloc()
  |
  v
virtual allocation
  |
  v
page not populated
  |
  v
first access
  |
  v
page fault
  |
  v
physical page
  |
  v
PTE update
  |
  v
instruction resumes
```

This is one of the most important combined interview flows.

---

# 72. Memory Allocation and TLB

After a virtual page is mapped:

```text
Virtual Page
     |
     v
Page table
     |
     v
Physical Frame
```

The translation may then be cached:

```text
Virtual Page
     |
     v
TLB
     |
     v
Physical Frame
```

Therefore memory allocation, page tables and TLB behavior are connected.

---

# 73. `malloc()` vs `mmap()` — Interview View

Question:

> When might an allocator use `mmap()`?

Answer:

> A general-purpose allocator can use different mechanisms depending on allocation size, allocator policy and implementation. Large allocations are commonly candidates for dedicated memory mappings, while smaller allocations may be served from allocator-managed arenas/heaps. Exact thresholds are implementation-dependent.

Avoid claiming a fixed threshold unless you are discussing a specific libc version/configuration.

---

# 74. `brk()` vs `mmap()` — Conceptual Difference

```text
brk()
 |
 +--> changes program break / heap-related region
```

```text
mmap()
 |
 +--> creates a mapping at a virtual address range
```

`mmap()` is much more general and is used for:

```text
anonymous memory
files
shared memory
shared libraries
special mappings
```

---

# 75. Memory Allocation in C++

C++ dynamic allocation:

```cpp
int *p = new int(10);

delete p;
```

Array:

```cpp
int *p = new int[10];

delete[] p;
```

Modern C++ should prefer:

```cpp
std::make_unique<T>()
std::make_shared<T>()
```

when ownership semantics allow it.

The underlying runtime may eventually use the process allocator, but C++ allocation and ownership semantics are separate concepts.

---

# 76. Placement New

Placement new constructs an object in already-provided storage.

```cpp
#include <new>
#include <cstdlib>

struct A
{
    int x;
};

int main()
{
    void *mem = std::malloc(sizeof(A));

    A *p = new (mem) A{10};

    p->~A();

    std::free(mem);
}
```

Important distinction:

```text
allocate storage
    !=
construct object
```

Placement new separates these operations.

---

# 77. Alignment-Aware Allocation

For special alignment requirements, C provides:

```c
aligned_alloc()
```

Example:

```c
void *p = aligned_alloc(64, 1024);
```

The size requirements of `aligned_alloc()` must satisfy the C standard/library requirements.

In C++ there are also aligned allocation facilities.

---

# 78. Why Alignment Matters

Alignment can matter for:

```text
CPU access efficiency
SIMD
cache-line alignment
DMA constraints
atomic operations
hardware interfaces
```

But alignment requirements are architecture/type dependent.

---

# 79. Cache-Line Alignment

Suppose two frequently modified variables share a cache line:

```text
CPU 0 -> variable A
CPU 1 -> variable B

same cache line
```

This can cause:

```text
cache-line bouncing
false sharing
```

Memory layout/alignment can therefore affect multithreaded performance.

This becomes especially important in high-performance systems.

---

# 80. NUMA and Memory Allocation

On NUMA systems:

```text
CPU 0 ---- Node 0 memory
CPU 1 ---- Node 1 memory
```

Memory access cost can depend on where the memory physically resides.

Conceptually:

```text
local memory
   |
   v
lower latency

remote node memory
   |
   v
higher latency
```

Linux provides NUMA-aware memory-management mechanisms.

For senior systems interviews, understand:

```text
allocation policy
CPU affinity
memory locality
remote access
```

---

# 81. Kernel `kmalloc()` vs `vmalloc()` — Strong Interview Answer

> `kmalloc()` is typically used when physically contiguous kernel memory is appropriate or required. `vmalloc()` provides a virtually contiguous kernel address range backed by potentially non-contiguous physical pages. `vmalloc()` therefore can handle larger allocations when physical contiguity is difficult, but it has additional mapping overhead and is not interchangeable with DMA-specific allocations.

---

# 82. Kernel `kmalloc()` vs `alloc_pages()`

```text
kmalloc()
    |
    v
byte-oriented/object-oriented kernel allocation
```

```text
alloc_pages()
    |
    v
page-oriented allocation
```

`alloc_pages()` works in page orders:

```text
2^order pages
```

Example:

```text
order 0 = 1 page
order 1 = 2 pages
order 2 = 4 pages
```

---

# 83. Kernel `kmalloc()` vs SLUB

They are different layers.

```text
Driver/subsystem
      |
      v
kmalloc()
      |
      v
SLUB
      |
      v
page allocator
      |
      v
buddy
```

`kmalloc()` is an allocation API.

SLUB is an implementation/layer used to efficiently manage kernel objects/small allocations.

---

# 84. What Is SLAB?

SLAB is a kernel object allocator design.

Conceptually:

```text
Page(s)
  |
  v
Cache
  |
  +--> object
  +--> object
  +--> object
```

Linux has historically had:

```text
SLAB
SLUB
SLOB
```

Modern mainstream Linux systems commonly use SLUB.

---

# 85. Object Reuse in SLUB

Suppose the kernel frequently creates:

```text
struct X
```

SLUB can maintain a cache for objects of that size.

```text
X cache
+----+----+----+----+
| X1 | X2 | X3 | X4 |
+----+----+----+----+
```

Free:

```text
X2
```

Later allocation:

```text
reuse X2
```

This reduces repeated low-level allocation work.

---

# 86. Memory Pressure

When memory becomes scarce:

```text
memory pressure
      |
      v
reclaim
      |
      +--> page cache
      +--> anonymous memory
      +--> slab memory
      +--> other reclaimable objects
```

The exact reclaim path depends on memory type and system state.

If reclaim cannot satisfy demand:

```text
OOM handling
```

may occur.

---

# 87. Page Cache vs Allocated Anonymous Memory

File-backed:

```text
file
 |
 v
page cache
 |
 v
mapping
```

Anonymous:

```text
process
 |
 v
anonymous page
```

Both can occupy physical RAM, but they have different backing and reclaim behavior.

---

# 88. Memory Accounting

Important terms:

```text
Virtual Size
RSS
PSS
Shared memory
Private memory
```

## Virtual Size

Total virtual address space represented by mappings.

## RSS

Resident pages currently associated with the process.

## PSS

Proportional Set Size, which accounts for shared pages by dividing their contribution among sharing processes.

For memory-debugging interviews, understand that:

```text
virtual size != physical memory consumption
```

---

# 89. Memory Mapping Example

Suppose:

```text
Virtual:
0x1000 -> Page A
0x2000 -> Page B
0x3000 -> Page C
```

Physical:

```text
Page A -> Frame 500
Page B -> Frame 100
Page C -> Frame 900
```

Virtual addresses remain contiguous:

```text
0x1000
0x2000
0x3000
```

Physical frames do not need to be contiguous.

This is the foundation of virtual memory.

---

# 90. Complete User-Space Allocation Flow

```text
Application
    |
    v
malloc(1 MB)
    |
    v
libc allocator
    |
    +--> reuse existing chunk?
    |       |
    |      yes
    |       |
    |       v
    |     return
    |
    no
    |
    v
request more virtual memory
    |
    +--> heap/program break
    |
    +--> mmap()
    |
    v
virtual address returned
    |
    v
application accesses memory
    |
    v
page fault if required
    |
    v
Linux VM
    |
    v
physical page
    |
    v
page-table mapping
    |
    v
instruction resumes
```

---

# 91. Complete Kernel Allocation Flow

```text
Kernel subsystem
       |
       v
kmalloc()
       |
       v
SLUB
       |
       +--> free object available
       |       |
       |       v
       |     return
       |
       +--> need backing memory
               |
               v
          page allocator
               |
               v
             buddy
               |
               v
        physical page(s)
```

For large virtually contiguous mappings:

```text
Kernel subsystem
       |
       v
vmalloc()
       |
       v
virtual mapping management
       |
       v
physical pages
       |
       v
page-table mappings
       |
       v
virtually contiguous result
```

---

# 92. Common Interview Traps

### Trap 1

> `malloc()` is a system call.

**Correct:** `malloc()` is a library allocator API.

### Trap 2

> Every malloc immediately allocates physical RAM.

**Correct:** virtual allocation and physical-page population can be separated.

### Trap 3

> `kmalloc()` and `vmalloc()` are equivalent.

**Correct:** physical-contiguity properties differ.

### Trap 4

> `vmalloc()` returns physically contiguous memory.

**Correct:** it provides virtually contiguous memory backed by potentially non-contiguous physical pages.

### Trap 5

> `kmalloc()` always means one page.

**Correct:** it can allocate different sizes; larger allocations have order/fragmentation constraints.

### Trap 6

> SLUB is the same thing as `kmalloc()`.

**Correct:** `kmalloc()` is an API; SLUB is a kernel allocator layer/implementation.

### Trap 7

> A page fault means a program crashed.

**Correct:** many page faults are recoverable and normal.

---

# 93. Senior Interview Questions

## Q1. What happens internally when `malloc()` is called?

Strong answer:

```text
application
   |
   v
malloc()
   |
   v
userspace allocator
   |
   +--> reuse existing chunk
   |
   +--> request more memory
           |
           +--> brk()
           +--> mmap()
```

The returned memory is virtual process memory. Physical-page population can happen lazily on first access.

---

## Q2. Why doesn't every malloc call enter the kernel?

Because the userspace allocator maintains its own memory pools/chunks/arenas and can satisfy many allocations from memory it already controls.

---

## Q3. What is the difference between `brk()` and `mmap()`?

```text
brk()
 -> changes program break / heap region

mmap()
 -> creates or modifies a virtual mapping
```

`mmap()` supports much broader use cases.

---

## Q4. Why is `mmap()` useful for large allocations?

A dedicated mapping can make management and eventual release convenient for certain allocation patterns. Exact allocator policy is implementation-specific.

---

## Q5. Difference between `kmalloc()` and `vmalloc()`?

```text
kmalloc:
virtually contiguous
physically contiguous

vmalloc:
virtually contiguous
physically non-contiguous allowed
```

---

## Q6. What is the buddy allocator?

> It manages physical memory in power-of-two page blocks, splitting larger blocks to satisfy smaller requests and merging free buddy blocks when possible.

---

## Q7. What is SLUB?

> SLUB is a slab-family kernel object allocator that efficiently manages frequently allocated fixed-size kernel objects and small allocations using caches backed by physical pages.

---

## Q8. Why can `vmalloc()` be slower than `kmalloc()`?

Because it may require additional virtual mapping/page-table work for physically non-contiguous pages.

---

## Q9. Why are GFP flags important?

They describe allocation context and constraints, including whether the allocator may sleep/reclaim and what memory requirements apply.

---

## Q10. What happens if kernel allocation fails?

The allocation API returns failure, typically `NULL` for pointer-returning APIs such as `kmalloc()`. The caller must handle failure appropriately.

---

# 94. Senior Scenario — Driver Needs a Buffer

Question:

> A driver needs a physically contiguous buffer. Which API might you consider?

Answer:

```text
kmalloc()
```

may be appropriate for suitable sizes/contexts, but if the memory is for DMA, use the Linux DMA API rather than assuming a generic kernel allocation is the correct device buffer.

For example:

```text
dma_alloc_coherent()
```

may be appropriate for a coherent DMA allocation depending on the device/use case.

---

# 95. Senior Scenario — Large Kernel Virtual Buffer

Question:

> You need a large virtually contiguous kernel buffer and physical contiguity is not required.

Conceptually:

```text
vmalloc()
```

may be appropriate.

But always consider:

```text
access pattern
DMA requirements
performance
allocation size
lifetime
```

---

# 96. Senior Scenario — Allocation in Atomic Context

Question:

> Can you blindly use `GFP_KERNEL` everywhere?

No.

If the current context cannot sleep, an allocation mode that can sleep is inappropriate.

You must choose flags compatible with the execution context.

---

# 97. Senior Scenario — Why Did RSS Increase After Touching Memory?

Possible explanation:

```text
virtual allocation
       |
       v
pages not resident
       |
       v
first access
       |
       v
page fault
       |
       v
physical page becomes resident
       |
       v
RSS increases
```

This demonstrates:

```text
virtual memory != resident physical memory
```

---

# 98. Senior Scenario — Why Does `free()` Not Necessarily Reduce Process RSS Immediately?

Because the userspace allocator may keep released chunks for future allocations instead of immediately returning all memory to the kernel.

Conceptually:

```text
free()
  |
  v
allocator free structure
  |
  v
reuse later
```

Returning memory to the kernel depends on allocator policy and mapping type.

---

# 99. Senior Scenario — Why Does a Process Have Huge VmSize but Smaller RSS?

Because a process can have many virtual mappings/pages that are not currently resident in physical memory.

```text
VmSize
  |
  +--> virtual address space

RSS
  |
  +--> resident memory
```

---

# 100. Chapter 9 One-Minute Revision

```text
malloc()
   ↓
userspace allocator
   ↓
reuse chunk OR obtain more memory
   ↓
brk()/mmap()
   ↓
virtual address space
   ↓
first access
   ↓
page fault
   ↓
physical page
   ↓
PTE
   ↓
TLB
   ↓
RAM
```

Kernel:

```text
kmalloc()
   ↓
SLUB
   ↓
page allocator
   ↓
buddy
   ↓
physical pages
```

Large virtually contiguous kernel memory:

```text
vmalloc()
   ↓
virtual mapping
   ↓
non-contiguous physical pages
```

---

# 101. Chapter 9 Cheat Sheet

| Concept | Key Point |
|---|---|
| `malloc()` | Userspace dynamic allocation API |
| `calloc()` | Allocation + zero initialization |
| `realloc()` | Resize allocation |
| `free()` | Release malloc-family allocation |
| `brk()` | Changes program break |
| `sbrk()` | Adjusts/queries program break |
| `mmap()` | Creates virtual memory mapping |
| `munmap()` | Removes mapping |
| Heap | Dynamic process memory region |
| Fragmentation | Wasted/scattered memory |
| Pool | Preallocated reusable blocks |
| `kmalloc()` | Kernel physically contiguous allocation for suitable sizes |
| `kzalloc()` | Zeroed kernel allocation |
| `kcalloc()` | Zeroed array allocation |
| `kfree()` | Free kmalloc-family memory |
| `vmalloc()` | Virtually contiguous kernel memory |
| `vfree()` | Free vmalloc memory |
| `alloc_pages()` | Page-oriented allocation |
| Buddy | Physical page allocator |
| SLAB/SLUB | Kernel object allocator |
| GFP flags | Allocation context/constraints |
| OOM | Out-of-memory handling |
| RSS | Resident memory |
| VmSize | Virtual address-space size |
| PSS | Proportional shared-memory accounting |
| DMA API | Device-memory allocation/mapping interface |

---

# 102. Final Mental Model

The complete memory-allocation picture is:

```text
                    USER SPACE
                        |
                        v
                   malloc()
                        |
                        v
                libc allocator
                        |
             +----------+----------+
             |                     |
          reuse                  request
             |                     |
             |               +-----+-----+
             |               |           |
             |             brk()       mmap()
             |               |           |
             +---------------+-----------+
                             |
                             v
                    Virtual Address
                             |
                             v
                     First Memory Access
                             |
                             v
                         Page Fault
                             |
                             v
                    Linux Memory Manager
                             |
                 +-----------+-----------+
                 |                       |
           anonymous page          file-backed page
                 |                       |
                 v                       v
             physical                page cache
               page                      |
                 |                       v
                 +----------+------------+
                            |
                            v
                         PTE
                            |
                            v
                           TLB
                            |
                            v
                           RAM
```

Kernel side:

```text
                KERNEL SPACE
                     |
          +----------+----------+
          |                     |
       kmalloc()             vmalloc()
          |                     |
         SLUB             virtual mappings
          |                     |
    page allocator         physical pages
          |                     |
        buddy                  |
          |                     |
          +----------+----------+
                     |
              Physical Memory
```

---

# 103. Chapter 9 Takeaways

1. Memory allocation must be understood at both user and kernel levels.
2. `malloc()` is a library allocator API, not a system call.
3. `malloc()` may use mechanisms such as `brk()` and `mmap()` internally.
4. The userspace allocator can satisfy many allocations without entering the kernel.
5. `malloc()` returns virtual process memory.
6. Physical page population can be lazy.
7. First access can trigger a page fault.
8. `calloc()` initializes allocated bytes to zero.
9. `realloc()` can move an allocation.
10. Always use a temporary pointer when handling `realloc()` failure.
11. `free()` makes the allocated object invalid.
12. Memory leaks, use-after-free, double-free and buffer overflows are major bugs.
13. Internal fragmentation is waste within allocated storage.
14. External fragmentation is scattered free space.
15. Memory pools are useful for predictable fixed-size allocations.
16. `mmap()` creates virtual mappings and is more general than heap growth.
17. `kmalloc()` is a kernel allocation API.
18. `kzalloc()` provides zeroed kernel memory.
19. `alloc_pages()` allocates physical pages by order.
20. The buddy allocator manages page blocks and supports splitting/merging.
21. SLUB efficiently manages kernel objects and small allocations.
22. `vmalloc()` provides virtually contiguous kernel memory backed by potentially non-contiguous physical pages.
23. `kmalloc()` and `vmalloc()` have different physical-contiguity properties.
24. DMA memory should be handled using appropriate Linux DMA APIs.
25. GFP flags describe allocation context and constraints.
26. `GFP_KERNEL` is commonly used where sleeping is permitted.
27. Kernel allocation APIs must be matched with the correct free API.
28. Memory pressure triggers reclaim and can eventually lead to OOM handling.
29. `VmSize` and RSS measure different aspects of process memory.
30. The complete connection is:

```text
malloc()
 ↓
userspace allocator
 ↓
virtual memory
 ↓
page fault
 ↓
physical page
 ↓
page table
 ↓
TLB
 ↓
RAM
```

---

# 104. Chapter 10 Preview — IPC

The next chapter will connect processes and threads with Linux IPC:

```text
Process A
   |
   +--> pipe
   |
   +--> FIFO
   |
   +--> shared memory
   |
   +--> message queue
   |
   +--> socket
   |
   +--> signal
   |
   +--> semaphore
   |
   v
Process B
```

It will cover:

```text
pipe()
FIFO
dup()/dup2()
shared memory
mmap()
POSIX shared memory
System V IPC
message queues
signals
semaphores
eventfd
Unix domain sockets
socketpair()
IPC synchronization
producer-consumer examples
C programs
Linux kernel IPC internals
interview questions
```
