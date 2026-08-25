
> **How to use this document:**  
> You don't need to memorize syntax perfectly. You need to write these skeletons confidently,
> explain every line while writing, and know the *why* behind every decision.
> That's what separates a senior from a junior in these interviews.

---

## Part 1: Linux Kernel Driver Patterns

These are the **highest probability** asks at hardware companies. Know these cold.

---

### 1.1 Platform Driver Skeleton (most common kernel ask)

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/io.h>

struct my_dev {
    void __iomem *base;     /* mapped register base */
    int irq;
    struct device *dev;
    /* add your private state here */
};

/* Called when device matches — your initialization goes here */
static int my_driver_probe(struct platform_device *pdev)
{
    struct my_dev *priv;
    struct resource *res;

    /* allocate driver-private data, tied to device lifetime */
    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->dev = &pdev->dev;

    /* get MMIO region from device tree / platform resources */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    priv->base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(priv->base))
        return PTR_ERR(priv->base);

    /* get IRQ number */
    priv->irq = platform_get_irq(pdev, 0);
    if (priv->irq < 0)
        return priv->irq;

    /* store private data so we can retrieve it in other callbacks */
    platform_set_drvdata(pdev, priv);

    dev_info(&pdev->dev, "my_driver probed successfully\n");
    return 0;
}

static int my_driver_remove(struct platform_device *pdev)
{
    /* devm_* resources are freed automatically — just do custom cleanup here */
    struct my_dev *priv = platform_get_drvdata(pdev);
    dev_info(priv->dev, "my_driver removed\n");
    return 0;
}

/* Device tree match table */
static const struct of_device_id my_driver_of_match[] = {
    { .compatible = "myvendor,mydevice" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, my_driver_of_match);

static struct platform_driver my_driver = {
    .probe  = my_driver_probe,
    .remove = my_driver_remove,
    .driver = {
        .name           = "my_driver",
        .of_match_table = my_driver_of_match,
    },
};

module_platform_driver(my_driver);  /* replaces module_init/exit boilerplate */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ankit");
MODULE_DESCRIPTION("Example platform driver");
```

**What interviewers will ask while you write this:**
- Why `devm_kzalloc` and not `kmalloc`? → devm resources are automatically freed on device detach/error unwind — no manual cleanup needed, no leak on probe failure.
- Why `IS_ERR(priv->base)` and not NULL check? → `ioremap` returns an error-encoded pointer on failure, not NULL.
- What does `platform_set_drvdata` do? → stores your private struct in the device so you can retrieve it in `remove`, IRQ handler, file ops, etc.
- What is `module_platform_driver`? → macro that generates `module_init`/`module_exit` that call `platform_driver_register`/`unregister`.

---

### 1.2 Interrupt Handler with Bottom-Half (Workqueue)

```c
#include <linux/interrupt.h>
#include <linux/workqueue.h>

struct my_dev {
    void __iomem *base;
    int irq;
    struct work_struct work;    /* bottom half */
    u32 status_snapshot;        /* data passed from top to bottom half */
};

/* TOP HALF — runs in interrupt context, must be fast, cannot sleep */
static irqreturn_t my_irq_handler(int irq, void *dev_id)
{
    struct my_dev *priv = dev_id;

    /* read and ACK the interrupt immediately */
    priv->status_snapshot = readl(priv->base + STATUS_REG);
    writel(INT_ACK, priv->base + STATUS_REG);

    /* defer slow work to process context */
    schedule_work(&priv->work);

    return IRQ_HANDLED;
}

/* BOTTOM HALF — runs in process context, can sleep, can call blocking APIs */
static void my_work_handler(struct work_struct *work)
{
    struct my_dev *priv = container_of(work, struct my_dev, work);

    /* do slow processing of priv->status_snapshot here */
    dev_info(priv->dev, "processing status: 0x%x\n", priv->status_snapshot);
}

static int my_driver_probe(struct platform_device *pdev)
{
    struct my_dev *priv;
    /* ... kzalloc, ioremap as above ... */

    INIT_WORK(&priv->work, my_work_handler);

    /* request IRQ — devm variant auto-frees on remove */
    return devm_request_irq(&pdev->dev, priv->irq, my_irq_handler,
                            IRQF_SHARED, "my_driver", priv);
}
```

**What interviewers will ask:**
- Why can't you sleep in the top half? → runs in interrupt context, no process context, no scheduler, stack is the IRQ stack.
- Top half vs bottom half — when to use workqueue vs tasklet vs softirq? → workqueue: can sleep, process context, use when you need to call blocking APIs. Tasklet: atomic context, cannot sleep, lighter than workqueue. Softirq: highest performance, runs in interrupt context, used by networking/block subsystems, requires careful design.
- What is `container_of`? → macro that computes the address of the enclosing struct from a pointer to one of its members. Critical pattern used everywhere in the kernel.
- Why `IRQF_SHARED`? → allows multiple devices to share the same IRQ line; your handler must check if the interrupt is from your device and return `IRQ_NONE` if not.

---

### 1.3 Character Device (cdev) with File Operations

```c
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "mydev"
#define BUF_SIZE    1024

struct my_dev {
    struct cdev cdev;
    dev_t devno;
    char kbuf[BUF_SIZE];
    size_t data_len;
    struct mutex lock;
};

static int my_open(struct inode *inode, struct file *filp)
{
    struct my_dev *priv = container_of(inode->i_cdev, struct my_dev, cdev);
    filp->private_data = priv;   /* store for read/write/ioctl */
    return 0;
}

static ssize_t my_read(struct file *filp, char __user *ubuf, size_t count, loff_t *ppos)
{
    struct my_dev *priv = filp->private_data;
    ssize_t ret;

    if (mutex_lock_interruptible(&priv->lock))
        return -ERESTARTSYS;

    count = min(count, priv->data_len);

    /* NEVER directly dereference a __user pointer — use copy_to_user */
    if (copy_to_user(ubuf, priv->kbuf, count)) {
        ret = -EFAULT;
        goto out;
    }
    ret = count;
out:
    mutex_unlock(&priv->lock);
    return ret;
}

static ssize_t my_write(struct file *filp, const char __user *ubuf, size_t count, loff_t *ppos)
{
    struct my_dev *priv = filp->private_data;
    ssize_t ret;

    if (count > BUF_SIZE)
        return -EINVAL;

    if (mutex_lock_interruptible(&priv->lock))
        return -ERESTARTSYS;

    if (copy_from_user(priv->kbuf, ubuf, count)) {
        ret = -EFAULT;
        goto out;
    }
    priv->data_len = count;
    ret = count;
out:
    mutex_unlock(&priv->lock);
    return ret;
}

static const struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open  = my_open,
    .read  = my_read,
    .write = my_write,
};
```

**What interviewers will ask:**
- Why `copy_to_user` / `copy_from_user` and not `memcpy`? → user pointers may be invalid, paged out, or point to kernel-mapped memory — these functions handle page faults safely and return bytes not copied on failure.
- Why `mutex_lock_interruptible` and not `mutex_lock`? → allows the lock wait to be interrupted by a signal, returning `-ERESTARTSYS` so the syscall can be restarted. Better UX for user-facing drivers.
- What is `__user`? → sparse annotation — marks a pointer as living in user address space. Helps catch unsafe direct dereferences at compile time with sparse checker.
- Why return `-EFAULT` when `copy_to_user` fails? → standard errno for bad user address.

---

### 1.4 Kernel Synchronization — Choosing the Right Primitive

```
DECISION TREE (know this cold):

Can your code sleep / block?
├── NO (interrupt handler, softirq, tasklet, atomic section)
│   ├── Protecting simple integer/pointer → atomic_t / atomic64_t
│   ├── Short critical section, multi-core → spinlock_t
│   └── Read-heavy, write-rare → rwlock_t (read_lock / write_lock)
│
└── YES (process context, workqueue, kernel thread)
    ├── General mutual exclusion → struct mutex
    ├── Read-heavy data structure → struct rw_semaphore
    ├── Wait for a condition → wait_queue_head_t + wait_event_interruptible()
    ├── One-time completion event → struct completion
    └── Lock-free read side, rare writes → RCU (rcu_read_lock / synchronize_rcu)
```

```c
/* Spinlock — use in interrupt context or when sleep is forbidden */
spinlock_t lock;
spin_lock_init(&lock);

unsigned long flags;
spin_lock_irqsave(&lock, flags);    /* disables local IRQs, saves flags */
/* critical section */
spin_unlock_irqrestore(&lock, flags);

/* --- */

/* Mutex — use in process context when you can sleep */
struct mutex lock;
mutex_init(&lock);
mutex_lock(&lock);
/* critical section */
mutex_unlock(&lock);

/* --- */

/* Wait queue — blocking wait for a condition */
DECLARE_WAIT_QUEUE_HEAD(wq);
int condition = 0;

/* waiter (process context) */
wait_event_interruptible(wq, condition != 0);

/* waker (can be interrupt handler) */
condition = 1;
wake_up_interruptible(&wq);

/* --- */

/* Completion — one-shot event synchronization */
struct completion done;
init_completion(&done);

/* thread A waits */
wait_for_completion(&done);

/* thread B signals */
complete(&done);
```

**Most common interview trap:** Using a mutex inside an interrupt handler → deadlock/BUG(). Always use spinlock in IRQ context.

---

### 1.5 RCU — Read-Copy-Update (asked at senior level)

```c
#include <linux/rcupdate.h>

struct config {
    int value;
    struct rcu_head rcu;   /* for deferred free */
};

static struct config __rcu *global_cfg;

/* READER — extremely fast, no locks, can run concurrently with writers */
void reader(void)
{
    struct config *cfg;

    rcu_read_lock();                    /* disables preemption, not a real lock */
    cfg = rcu_dereference(global_cfg); /* safe pointer dereference under RCU */
    if (cfg)
        printk("value = %d\n", cfg->value);
    rcu_read_unlock();
    /* cfg must NOT be used after rcu_read_unlock */
}

/* WRITER — makes a new copy, swaps pointer, waits for readers to finish */
void writer(int new_value)
{
    struct config *new_cfg, *old_cfg;

    new_cfg = kmalloc(sizeof(*new_cfg), GFP_KERNEL);
    new_cfg->value = new_value;

    old_cfg = rcu_dereference_protected(global_cfg, lockdep_is_held(&my_lock));
    rcu_assign_pointer(global_cfg, new_cfg); /* atomic pointer swap */

    synchronize_rcu();   /* wait until ALL current readers exit their read-side section */
    kfree(old_cfg);      /* safe to free now — no reader can hold old_cfg anymore */
}
```

**Why RCU:** Reader side has near-zero overhead (no lock, just preemption disable). Perfect for frequently-read, rarely-written data (routing tables, driver config, security credentials).

---

## Part 2: C Patterns — Written From Scratch in Interviews

---

### 2.1 Linked List Operations (always asked)

```c
#include <stdio.h>
#include <stdlib.h>

typedef struct node {
    int data;
    struct node *next;
} node_t;

/* Insert at head — O(1) */
node_t *push(node_t *head, int val)
{
    node_t *n = malloc(sizeof(*n));
    n->data = val;
    n->next = head;
    return n;   /* new head */
}

/* Reverse in-place — O(n), O(1) space */
node_t *reverse(node_t *head)
{
    node_t *prev = NULL, *curr = head, *next;
    while (curr) {
        next = curr->next;
        curr->next = prev;
        prev = curr;
        curr = next;
    }
    return prev;  /* new head */
}

/* Detect cycle — Floyd's tortoise and hare */
int has_cycle(node_t *head)
{
    node_t *slow = head, *fast = head;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) return 1;
    }
    return 0;
}

/* Find middle node — slow/fast pointer */
node_t *find_middle(node_t *head)
{
    node_t *slow = head, *fast = head;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
    }
    return slow;
}

/* Free entire list */
void free_list(node_t *head)
{
    node_t *tmp;
    while (head) {
        tmp = head->next;
        free(head);
        head = tmp;
    }
}
```

---

### 2.2 Bit Manipulation (very common at hardware companies)

```c
#include <stdint.h>
#include <stdio.h>

/* Check if bit N is set */
int bit_is_set(uint32_t val, int n)    { return (val >> n) & 1; }

/* Set bit N */
uint32_t set_bit(uint32_t val, int n)  { return val | (1U << n); }

/* Clear bit N */
uint32_t clr_bit(uint32_t val, int n)  { return val & ~(1U << n); }

/* Toggle bit N */
uint32_t tog_bit(uint32_t val, int n)  { return val ^ (1U << n); }

/* Count set bits (Kernighan's method) */
int count_bits(uint32_t val)
{
    int count = 0;
    while (val) {
        val &= val - 1;  /* clears the lowest set bit */
        count++;
    }
    return count;
}

/* Check power of two */
int is_power_of_two(uint32_t val) { return val && !(val & (val - 1)); }

/* Swap without temp variable */
void swap(int *a, int *b) { *a ^= *b; *b ^= *a; *a ^= *b; }

/* Extract bit field: bits [hi:lo] from val */
uint32_t extract_field(uint32_t val, int hi, int lo)
{
    uint32_t mask = (1U << (hi - lo + 1)) - 1;
    return (val >> lo) & mask;
}

/* Endianness swap (32-bit) */
uint32_t bswap32(uint32_t val)
{
    return ((val & 0xFF000000) >> 24) |
           ((val & 0x00FF0000) >>  8) |
           ((val & 0x0000FF00) <<  8) |
           ((val & 0x000000FF) << 24);
}

/* Runtime endianness detection */
int is_little_endian(void)
{
    uint32_t x = 1;
    return *(uint8_t *)&x == 1;
}
```

**Interview context:** At Qualcomm/AMD/Intel, bit manipulation comes up in register access, protocol parsing, DMA descriptor setup. Know these without hesitation.

---

### 2.3 Custom `memcpy` and `memset`

```c
/* memcpy — undefined behavior on overlap, use memmove for overlap */
void *my_memcpy(void *dest, const void *src, size_t n)
{
    uint8_t *d = dest;
    const uint8_t *s = src;
    while (n--)
        *d++ = *s++;
    return dest;
}

/* memmove — handles overlap correctly */
void *my_memmove(void *dest, const void *src, size_t n)
{
    uint8_t *d = dest;
    const uint8_t *s = src;
    if (d < s) {
        while (n--) *d++ = *s++;         /* forward copy */
    } else {
        d += n; s += n;
        while (n--) *--d = *--s;         /* backward copy */
    }
    return dest;
}

void *my_memset(void *dest, int c, size_t n)
{
    uint8_t *d = dest;
    while (n--)
        *d++ = (uint8_t)c;
    return dest;
}
```

**Follow-up interviewers always ask:** Why does `memcpy` have undefined behavior on overlap? Because the C standard allows implementations to copy in any order or use SIMD — copying forward and backward may give different results on overlapping regions.

---

### 2.4 Stack and Queue in C (from scratch)

```c
/* Stack using array */
#define MAX 256
typedef struct {
    int data[MAX];
    int top;
} Stack;

void stack_init(Stack *s)       { s->top = -1; }
int  stack_empty(Stack *s)      { return s->top == -1; }
void stack_push(Stack *s, int v){ s->data[++s->top] = v; }
int  stack_pop(Stack *s)        { return s->data[s->top--]; }
int  stack_peek(Stack *s)       { return s->data[s->top]; }

/* Queue using circular buffer */
typedef struct {
    int data[MAX];
    int head, tail, count;
} Queue;

void queue_init(Queue *q)       { q->head = q->tail = q->count = 0; }
int  queue_empty(Queue *q)      { return q->count == 0; }
void queue_enqueue(Queue *q, int v) {
    q->data[q->tail] = v;
    q->tail = (q->tail + 1) % MAX;
    q->count++;
}
int  queue_dequeue(Queue *q) {
    int v = q->data[q->head];
    q->head = (q->head + 1) % MAX;
    q->count--;
    return v;
}
```

---

### 2.5 Producer-Consumer (must write from memory)

```c
#include <pthread.h>
#include <stdio.h>

#define BUF_SIZE 8

static int buffer[BUF_SIZE];
static int count = 0, in = 0, out = 0;
static pthread_mutex_t lock     = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t not_full  = PTHREAD_COND_INITIALIZER;
static pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

void produce(int item)
{
    pthread_mutex_lock(&lock);
    while (count == BUF_SIZE)              /* WHILE not IF — spurious wakeups */
        pthread_cond_wait(&not_full, &lock);
    buffer[in] = item;
    in = (in + 1) % BUF_SIZE;
    count++;
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&lock);
}

int consume(void)
{
    int item;
    pthread_mutex_lock(&lock);
    while (count == 0)
        pthread_cond_wait(&not_empty, &lock);
    item = buffer[out];
    out = (out + 1) % BUF_SIZE;
    count--;
    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&lock);
    return item;
}
```

---

### 2.6 Deadlock: Show It, Fix It

```c
/* BUGGY — inconsistent lock order causes deadlock */
pthread_mutex_t A = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t B = PTHREAD_MUTEX_INITIALIZER;

void *thread1(void *arg) {
    pthread_mutex_lock(&A);   /* locks A first */
    sleep(1);
    pthread_mutex_lock(&B);   /* waits for B — but thread2 holds B */
    /* ... */
    pthread_mutex_unlock(&B);
    pthread_mutex_unlock(&A);
    return NULL;
}

void *thread2(void *arg) {
    pthread_mutex_lock(&B);   /* locks B first */
    sleep(1);
    pthread_mutex_lock(&A);   /* waits for A — but thread1 holds A => DEADLOCK */
    /* ... */
    pthread_mutex_unlock(&A);
    pthread_mutex_unlock(&B);
    return NULL;
}

/* FIX — enforce global lock ordering: always acquire A before B */
void *thread2_fixed(void *arg) {
    pthread_mutex_lock(&A);   /* same order as thread1 */
    pthread_mutex_lock(&B);
    /* ... */
    pthread_mutex_unlock(&B);
    pthread_mutex_unlock(&A);
    return NULL;
}
```

---

## Part 3: Process & IPC Patterns (write from memory)

---

### 3.1 fork + exec + waitpid (shell pattern)

```c
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

pid_t pid = fork();

if (pid < 0) {
    perror("fork"); exit(1);
}
if (pid == 0) {
    /* child */
    execlp("ls", "ls", "-l", (char *)NULL);
    perror("execlp");   /* only reached on exec failure */
    _exit(127);         /* _exit not exit — don't flush parent's stdio buffers */
}
/* parent */
int status;
waitpid(pid, &status, 0);
if (WIFEXITED(status))
    printf("child exited: %d\n", WEXITSTATUS(status));
else if (WIFSIGNALED(status))
    printf("child killed by signal: %d\n", WTERMSIG(status));
```

**Key distinction:** `_exit()` vs `exit()` — after `fork()`, child must use `_exit()` to avoid flushing the parent's stdio buffers (which are shared after fork before exec).

---

### 3.2 Pipe: Parent ↔ Child IPC

```c
int fds[2];
pipe(fds);  /* fds[0]=read end, fds[1]=write end */

pid_t pid = fork();
if (pid == 0) {
    /* child writes */
    close(fds[0]);                          /* close unused read end */
    write(fds[1], "hello", 5);
    close(fds[1]);
    _exit(0);
}
/* parent reads */
close(fds[1]);                              /* close unused write end */
char buf[32];
ssize_t n = read(fds[0], buf, sizeof(buf));
buf[n] = '\0';
printf("got: %s\n", buf);
close(fds[0]);
waitpid(pid, NULL, 0);
```

**Common interview trap:** Not closing the unused end → reader never sees EOF because the write end is still open somewhere.

---

### 3.3 Signal Handler (production-safe pattern)

```c
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

static volatile sig_atomic_t running = 1;   /* the ONLY safe type to write in a handler */

static void handle_shutdown(int signo)
{
    running = 0;    /* just flip a flag */
                    /* DO NOT: printf, malloc, mutex_lock — not async-signal-safe */
}

int main(void)
{
    struct sigaction sa = {0};
    sa.sa_handler = handle_shutdown;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;    /* restart interrupted syscalls automatically */

    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    while (running)
        sleep(1);

    printf("shutting down cleanly\n");
    return 0;
}
```

**What interviewers ask:** Why `sig_atomic_t`? → guaranteed to be read/written atomically on the target architecture, so no torn reads between signal delivery and main loop check. Why not `printf` in the handler? → not async-signal-safe (may call `malloc` internally, take locks that could be held when the signal fired → deadlock).

---

### 3.4 Zombie and Orphan — Show with Code

```c
/* Zombie: child exits, parent doesn't call wait() */
pid_t pid = fork();
if (pid == 0) {
    _exit(0);          /* child exits immediately */
}
sleep(10);             /* parent sleeps — child is now a zombie in process table */
wait(NULL);            /* reaps it — zombie disappears */

/* --- */

/* Orphan: parent exits before child */
pid_t pid = fork();
if (pid == 0) {
    sleep(5);          /* child still running */
    printf("my new parent is PID %d\n", getppid()); /* will be 1 (init/systemd) */
    _exit(0);
}
_exit(0);   /* parent exits immediately — child becomes orphan, reparented to PID 1 */
```

---

## Part 4: Memory & Pointer Patterns

---

### 4.1 Common Pointer Bugs (identify and fix these)

```c
/* BUG 1: Use after free */
int *p = malloc(sizeof(int));
free(p);
*p = 5;       /* UNDEFINED — p is dangling */

/* FIX */
free(p);
p = NULL;     /* always NULL after free */

/* --- */

/* BUG 2: Double free */
free(p);
free(p);      /* UNDEFINED — corrupts allocator state */

/* --- */

/* BUG 3: Buffer overflow */
char buf[8];
strcpy(buf, "this is too long");  /* overflows — use strncpy or snprintf */

/* --- */

/* BUG 4: Returning pointer to local variable */
int *bad(void) {
    int x = 42;
    return &x;   /* x lives on stack — undefined after function returns */
}

/* --- */

/* BUG 5: sizeof pointer vs array */
void f(int arr[]) {
    int n = sizeof(arr) / sizeof(arr[0]);  /* WRONG — arr decays to pointer */
}
void f_correct(int *arr, size_t n) { /* pass size explicitly */ }
```

---

### 4.2 Volatile — When and Why

```c
/* 1. Hardware register — compiler must not cache the read */
volatile uint32_t *reg = (volatile uint32_t *)0xDEAD0000;
uint32_t status = *reg;   /* always reads from address, never from register cache */

/* 2. Signal handler flag */
static volatile sig_atomic_t flag = 0;

/* 3. Shared variable in embedded polling loop */
volatile int done = 0;
while (!done) { }   /* without volatile, compiler may optimize to infinite loop */

/* What volatile does NOT do: it does NOT provide atomicity or memory ordering.
   For multi-threaded code use atomic_t (kernel) or stdatomic.h (userspace). */
```

---

### 4.3 Memory Layout Quiz (draw this when asked)

```
High Address
┌─────────────────┐
│   Kernel space  │  (not accessible from user mode)
├─────────────────┤  ← user stack top (grows ↓)
│   Stack         │  local variables, function frames, return addresses
│       ↓         │
│   [guard page]  │
│       ↑         │
│   Heap          │  malloc/free (grows ↑)
├─────────────────┤
│   BSS           │  uninitialized / zero-initialized globals and statics
├─────────────────┤
│   Data          │  initialized globals and statics
├─────────────────┤
│   Text (code)   │  read-only executable instructions
└─────────────────┘
Low Address (0x0 — NULL)
```

---

## Part 5: Kernel Internals — Explain + Sketch

These are verbal/whiteboard questions. Know the flow well enough to draw it.

---

### 5.1 System Call Flow

```
User process calls read()
        │
        ▼
  C library (glibc)
  puts syscall number in %rax
  arguments in %rdi, %rsi, %rdx ...
  executes `syscall` instruction
        │
        ▼  (CPU switches to kernel mode, saves user registers)
  syscall entry point (entry_SYSCALL_64)
        │
        ▼
  sys_call_table[__NR_read]  →  ksys_read()
        │
        ▼
  VFS layer → file->f_op->read() → driver read()
        │
        ▼
  return value placed in %rax
  `sysret` instruction — CPU switches back to user mode
        │
        ▼
  glibc sets errno if return < 0, returns to caller
```

---

### 5.2 Page Fault Flow

```
CPU accesses virtual address
        │
        ▼
  MMU walks page tables
  Page not present → page fault exception
        │
        ▼
  do_page_fault() in kernel
        │
   ┌────┴──────────────────────────────┐
   │                                   │
Valid address (VMA exists)?          Invalid address?
   │                                   │
   ▼                                   ▼
Page in from disk/swap           SIGSEGV to process
(demand paging / CoW)
   │
   ▼
Update page table, retry instruction
```

---

### 5.3 Context Switch Steps

```
Running: Process A
    │
    ▼  (timer interrupt / voluntary yield / blocking syscall)
Save A's state into task_struct:
  - CPU registers (including %rsp, %rip via SAVE_ALL)
  - FPU/SIMD state (lazy)
  - memory management context (mm_struct)
    │
    ▼
Scheduler picks Process B
    │
    ▼
switch_mm(): switch CR3 (page table base) if different address space
    │
    ▼
switch_to(): restore B's registers from its task_struct
    │
    ▼
Running: Process B (resumes from where it left off)
```

---

### 5.4 Spinlock vs Mutex — Decision in One Sentence

| Scenario | Use |
|---|---|
| Interrupt handler, softirq, atomic section | `spinlock` + `spin_lock_irqsave` |
| Process context, short critical section, SMP | `spinlock` or `mutex` |
| Process context, can sleep, long hold time | `mutex` |
| Read-heavy, writes rare, process context | `rw_semaphore` |
| Frequently-read config, rare updates | `RCU` |
| Wait for async event | `wait_queue` or `completion` |

---

## Part 6: Device Tree (asked at Qualcomm/ARM/Broadcom)

```dts
/* Minimal DTS node for your platform driver */
mydevice@40010000 {
    compatible = "myvendor,mydevice";   /* must match of_match_table */
    reg = <0x40010000 0x1000>;          /* MMIO base address, size */
    interrupts = <0 42 IRQ_TYPE_LEVEL_HIGH>;
    clocks = <&clk_gate 3>;
    clock-names = "core";
    status = "okay";
};
```

```c
/* Reading a custom DT property in probe() */
u32 timeout;
if (of_property_read_u32(pdev->dev.of_node, "myvendor,timeout-ms", &timeout))
    timeout = 100;  /* default if property absent */
```

---

## Part 7: Interview Conversation Patterns

These are the verbal answers interviewers expect at senior level.

---

### "Walk me through what happens when you write a driver for a new peripheral."

> "First I look at the datasheet to understand the register map, interrupt behavior, and DMA capabilities. Then I check if there's an existing subsystem that fits — for example, if it's a serial device I'd use the UART framework rather than a raw char driver. I write a platform driver with `probe`/`remove`, use `devm_*` APIs for all resource allocation so cleanup is automatic. I set up interrupt handling — top half just reads status and ACKs the interrupt, schedules a workqueue for anything that can sleep. I add sysfs or debugfs nodes for observability. Then I write a simple test using `/dev` or sysfs from userspace and use `ftrace`/`perf` to verify the interrupt path latency."

---

### "How would you debug a driver that works on single-core but fails on SMP?"

> "Classic sign of a locking bug. I'd first run with `CONFIG_DEBUG_SPINLOCK` and `CONFIG_LOCKDEP` enabled — lockdep will usually catch lock ordering violations or missing locks immediately. I'd look for any shared state accessed without proper synchronization — especially anything that's 'obviously safe' like a single pointer or integer, which may not be atomic on all architectures. I'd also check for interrupt context issues — a mutex being taken in a code path that can be called from an IRQ handler."

---

### "Difference between `kmalloc` and `vmalloc`?"

> "`kmalloc` allocates physically contiguous memory — required for DMA, fast, limited to a few MB. `vmalloc` allocates virtually contiguous but physically scattered pages — for large allocations where physical contiguity isn't needed, like loading a firmware image. `kzalloc` is just `kmalloc` + memset to zero. For DMA you'd use `dma_alloc_coherent` which gives you physically contiguous memory with proper cache coherency guarantees."

---

### "What's the difference between a process and a thread in Linux?"

> "In Linux, both are tasks — created with `clone()` with different sharing flags. A thread shares the address space, file descriptors, and signal handlers with its parent. A process gets its own copy (via CoW for memory). From the scheduler's perspective they're identical — both are `task_struct`. The distinction is purely what they share."

---

## Part 8: Quick-Reference Cheat Sheet

### Kernel memory allocation

| Function | Physically contiguous | Sleepable | Use for |
|---|---|---|---|
| `kmalloc(size, GFP_KERNEL)` | Yes | Yes | Small kernel objects |
| `kmalloc(size, GFP_ATOMIC)` | Yes | No | IRQ context |
| `kzalloc(size, GFP_KERNEL)` | Yes | Yes | Same + zeroed |
| `vmalloc(size)` | No | Yes | Large, non-DMA buffers |
| `dma_alloc_coherent()` | Yes | Yes | DMA transfers |
| `devm_kzalloc()` | Yes | Yes | Device-managed, auto-freed |

### GFP flags

| Flag | Meaning |
|---|---|
| `GFP_KERNEL` | Normal allocation, can sleep |
| `GFP_ATOMIC` | Interrupt context, cannot sleep, may fail |
| `GFP_DMA` | Must be in DMA-able zone |
| `GFP_ZERO` | Zero the memory |

### Most-used kernel APIs

```c
/* Register access */
readl(base + OFFSET)          /* read 32-bit MMIO register */
writel(val, base + OFFSET)    /* write 32-bit MMIO register */

/* Delays */
udelay(n)      /* busy-wait microseconds — for very short delays in any context */
msleep(n)      /* sleep milliseconds — process context only */
usleep_range(min, max)        /* preferred over udelay for > ~10us */

/* Logging */
dev_err(dev, "fmt", ...)      /* pr_err + device name prefix */
dev_info(dev, "fmt", ...)
dev_dbg(dev, "fmt", ...)      /* compiled out unless DEBUG defined */

/* Assertions */
BUG_ON(condition)             /* oops + stack trace if true */
WARN_ON(condition)            /* stack trace but continues */
```

### Errno values to know

| Errno | Meaning |
|---|---|
| `-ENOMEM` | Out of memory |
| `-EINVAL` | Invalid argument |
| `-ENODEV` | No such device |
| `-EBUSY` | Device or resource busy |
| `-EFAULT` | Bad user address (copy_to/from_user failed) |
| `-ERESTARTSYS` | Interrupted, syscall should restart |
| `-ETIMEDOUT` | Timeout |
| `-EIO` | I/O error |

---

## Appendix: What NOT to Waste Time On

At your experience level and for these companies, **skip**:

- LeetCode dynamic programming (not asked at Qualcomm/AMD/Intel/ARM for driver roles)
- Sorting algorithm implementations (know the names and complexity, don't implement)
- Binary tree traversals (rarely asked, and trivial if it does come up)
- Complex graph algorithms

**Focus time here:**
1. Platform driver skeleton — write it cold ✓
2. IRQ handler + workqueue — write it cold ✓
3. `copy_to_user` / `copy_from_user` pattern — write it cold ✓
4. Spinlock vs mutex decision — instant answer ✓
5. Producer-consumer with condition variable — write it cold ✓
6. Bit manipulation — all operations in ~2 minutes ✓
7. `fork` + `exec` + `waitpid` — write it cold ✓
8. Signal handler skeleton — write it cold ✓
