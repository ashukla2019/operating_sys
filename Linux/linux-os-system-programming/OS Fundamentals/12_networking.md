# Chapter 12 — Networking

> **Three-layer approach**
>
> This chapter covers:
> 1. **[OS] Networking fundamentals**
> 2. **[LSP] Linux System Programming + C code**
> 3. **[KERNEL] Linux Kernel Internals**
>
> Core flow:
>
> ```text
> Application
>      ↓
> socket API
>      ↓
> Socket layer
>      ↓
> TCP / UDP
>      ↓
> IP
>      ↓
> Routing / qdisc
>      ↓
> NIC driver
>      ↓
> NIC
>      ↓
> Network
> ```

---

# 1. What Is Computer Networking?

Networking allows processes running on different systems to exchange data.

At a high level:

```text
Application A
     |
     v
Network stack
     |
     v
Network
     |
     v
Network stack
     |
     v
Application B
```

Linux exposes networking to applications primarily through the **socket API**.

---

# 2. OS View of Networking

From an OS perspective, networking involves:

```text
processes
   ↓
system calls
   ↓
kernel networking stack
   ↓
device driver
   ↓
NIC
   ↓
physical/network medium
```

The OS provides:

```text
process isolation
buffering
scheduling
memory management
socket abstraction
network protocol implementation
device-driver interface
```

---

# 3. Network Models

Two common models are:

```text
OSI model
TCP/IP model
```

For Linux programming, the TCP/IP model is usually more practical.

Simplified:

```text
Application
     ↓
Transport
     ↓
Internet
     ↓
Link
     ↓
Physical
```

Examples:

```text
Application → HTTP, DNS, SSH
Transport  → TCP, UDP
Internet   → IP
Link       → Ethernet
Physical   → electrical/optical/radio signaling
```

---

# 4. TCP/IP Stack

Typical flow:

```text
Application
    ↓
TCP / UDP
    ↓
IP
    ↓
Ethernet / Wi-Fi
    ↓
NIC
```

On receive:

```text
NIC
    ↓
Ethernet
    ↓
IP
    ↓
TCP / UDP
    ↓
Socket
    ↓
Application
```

---

# 5. MAC Address vs IP Address vs Port

These are different concepts.

### MAC address

Identifies a network interface at the link layer.

Example:

```text
00:11:22:33:44:55
```

### IP address

Identifies an endpoint/interface at the IP layer.

Example:

```text
192.168.1.10
```

### Port

Identifies a transport-layer endpoint associated with a process/service.

Example:

```text
TCP port 8080
```

Think:

```text
MAC → local network interface
IP  → network-layer endpoint
Port → transport/application endpoint
```

---

# 6. IPv4 Address

IPv4 is 32 bits.

Example:

```text
192.168.1.10
```

Represented as four octets:

```text
192 . 168 . 1 . 10
```

Total:

```text
4 bytes = 32 bits
```

---

# 7. IPv6 Address

IPv6 uses 128-bit addresses.

Example:

```text
2001:db8::1
```

Advantages include:

```text
much larger address space
autoconfiguration features
modern networking capabilities
```

Linux socket programming can support both IPv4 and IPv6.

---

# 8. Port Number

TCP and UDP use 16-bit port numbers.

Conceptually:

```text
IP address + transport protocol + port
```

identifies a communication endpoint.

Examples:

```text
22   → SSH
53   → DNS
80   → HTTP
443  → HTTPS
```

Ports are conventions, not guarantees that a particular process is actually running that service.

---

# 9. TCP

TCP is:

```text
connection-oriented
reliable
ordered
byte-stream based
```

It provides mechanisms for:

```text
sequence numbers
acknowledgements
retransmission
flow control
congestion control
connection management
```

---

# 10. TCP Is a Byte Stream

This is a critical interview point.

Suppose sender does:

```c
send(fd, "HELLO", 5, 0);
send(fd, "WORLD", 5, 0);
```

Receiver is not guaranteed to receive:

```text
HELLO
WORLD
```

It could receive:

```text
HELLOWORLD
```

or:

```text
HEL
LOWOR
LD
```

Therefore applications must define their own message framing.

---

# 11. TCP Message Framing

Common techniques:

### Fixed size

```text
every message = 100 bytes
```

### Length prefix

```text
[length][payload]
```

Example:

```text
[0005][HELLO]
```

### Delimiter

```text
HELLO\n
WORLD\n
```

### Self-describing protocol

For example:

```text
HTTP
protobuf
custom binary protocol
```

---

# 12. UDP

UDP is:

```text
connectionless
datagram-oriented
no built-in reliability
message preserving
```

It does not provide TCP-style:

```text
retransmission
ordering
connection establishment
```

Applications can implement reliability themselves when required.

---

# 13. TCP vs UDP

| Feature | TCP | UDP |
|---|---|---|
| Connection | Yes | No |
| Reliable delivery | Yes | No |
| Ordering | Yes | No |
| Byte stream | Yes | No |
| Datagram/message boundaries | No | Yes |
| Retransmission | Built-in | Application-dependent |
| Flow control | Yes | No TCP-style flow control |
| Congestion control | Yes | No TCP-style mechanism |
| Typical uses | HTTP, SSH, databases | DNS, streaming, telemetry, real-time traffic |

---

# 14. TCP Connection Lifecycle

Server:

```text
socket()
   ↓
bind()
   ↓
listen()
   ↓
accept()
```

Client:

```text
socket()
   ↓
connect()
```

Connection establishment:

```text
Client                     Server

  SYN  -------------------->

       <------------------- SYN + ACK

  ACK  -------------------->
```

This is the TCP three-way handshake.

---

# 15. TCP Server Architecture

Basic server:

```text
socket()
   ↓
bind()
   ↓
listen()
   ↓
accept()
   ↓
recv()/read()
   ↓
send()/write()
   ↓
close()
```

Important:

> `listen()` creates a listening socket; `accept()` returns a new connected socket.

---

# 16. TCP Client Architecture

```text
socket()
   ↓
connect()
   ↓
send()/write()
   ↓
recv()/read()
   ↓
close()
```

---

# 17. `socket()`

Create a socket:

```c
int fd = socket(AF_INET,
                SOCK_STREAM,
                0);
```

Parameters:

```text
AF_INET
    → IPv4

SOCK_STREAM
    → TCP-style stream socket

0
    → choose appropriate protocol
```

For UDP:

```c
socket(AF_INET, SOCK_DGRAM, 0);
```

---

# 18. `socket()` Example

```c
#include <stdio.h>
#include <sys/socket.h>

int main(void)
{
    int fd = socket(AF_INET,
                    SOCK_STREAM,
                    0);

    if (fd == -1)
    {
        perror("socket");
        return 1;
    }

    printf("socket fd = %d\n", fd);

    return 0;
}
```

Remember to close it in real programs:

```c
close(fd);
```

---

# 19. Address Structures

IPv4 commonly uses:

```c
struct sockaddr_in
```

Example:

```c
struct sockaddr_in addr = {0};

addr.sin_family = AF_INET;
addr.sin_port = htons(8080);
addr.sin_addr.s_addr = htonl(INADDR_ANY);
```

---

# 20. Network Byte Order

Networks commonly use **big-endian/network byte order**.

Functions:

```c
htons()
htonl()
ntohs()
ntohl()
```

Meaning:

```text
h → host
n → network
s → 16-bit
l → 32-bit
```

Examples:

```c
htons(port);
htonl(ip);
ntohs(port);
ntohl(ip);
```

---

# 21. `inet_pton()`

Convert textual IP to binary representation.

```c
inet_pton(AF_INET,
          "192.168.1.10",
          &addr.sin_addr);
```

Prefer `inet_pton()` over older APIs such as `inet_addr()` for modern code.

---

# 22. `bind()`

Associate a socket with a local address/port.

```c
bind(fd,
     (struct sockaddr *)&addr,
     sizeof(addr));
```

Typical server:

```text
socket
  ↓
bind
  ↓
listen
```

---

# 23. `bind()` Example

```c
struct sockaddr_in addr = {0};

addr.sin_family = AF_INET;
addr.sin_port = htons(8080);
addr.sin_addr.s_addr = htonl(INADDR_ANY);

if (bind(fd,
         (struct sockaddr *)&addr,
         sizeof(addr)) == -1)
{
    perror("bind");
}
```

---

# 24. `listen()`

Convert a stream socket into a listening socket:

```c
listen(fd, 128);
```

The backlog relates to pending connection handling.

Conceptually:

```text
client connections
       ↓
listen socket
       ↓
pending connection queues
```

---

# 25. `accept()`

Accept a pending connection:

```c
int client_fd = accept(server_fd,
                       NULL,
                       NULL);
```

Important:

```text
server_fd → listening socket
client_fd → connected socket
```

The listening socket normally remains available for additional connections.

---

# 26. TCP Server — Complete Basic Example

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(void)
{
    int server_fd = socket(AF_INET,
                           SOCK_STREAM,
                           0);

    if (server_fd == -1)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;

    setsockopt(server_fd,
               SOL_SOCKET,
               SO_REUSEADDR,
               &opt,
               sizeof(opt));

    struct sockaddr_in addr = {0};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd,
             (struct sockaddr *)&addr,
             sizeof(addr)) == -1)
    {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 16) == -1)
    {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Server listening on port 8080\n");

    int client_fd = accept(server_fd, NULL, NULL);

    if (client_fd == -1)
    {
        perror("accept");
        close(server_fd);
        return 1;
    }

    char buffer[1024];

    ssize_t n = recv(client_fd,
                     buffer,
                     sizeof(buffer) - 1,
                     0);

    if (n > 0)
    {
        buffer[n] = '\0';

        printf("Received: %s\n", buffer);

        send(client_fd,
             buffer,
             n,
             0);
    }

    close(client_fd);
    close(server_fd);

    return 0;
}
```

Compile:

```bash
gcc server.c -o server
```

Run:

```bash
./server
```

---

# 27. TCP Client — Complete Basic Example

```c
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(void)
{
    int fd = socket(AF_INET,
                    SOCK_STREAM,
                    0);

    if (fd == -1)
    {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server = {0};

    server.sin_family = AF_INET;
    server.sin_port = htons(8080);

    if (inet_pton(AF_INET,
                  "127.0.0.1",
                  &server.sin_addr) != 1)
    {
        perror("inet_pton");
        close(fd);
        return 1;
    }

    if (connect(fd,
                (struct sockaddr *)&server,
                sizeof(server)) == -1)
    {
        perror("connect");
        close(fd);
        return 1;
    }

    const char *msg = "Hello server";

    send(fd,
         msg,
         strlen(msg),
         0);

    char buffer[1024];

    ssize_t n = recv(fd,
                     buffer,
                     sizeof(buffer) - 1,
                     0);

    if (n > 0)
    {
        buffer[n] = '\0';
        printf("Reply: %s\n", buffer);
    }

    close(fd);

    return 0;
}
```

---

# 28. Why Basic TCP Server Is Not Production-Ready

The example handles:

```text
one client
one recv
one send
```

Real servers need:

```text
multiple clients
partial reads
partial writes
connection closure
errors
timeouts
non-blocking sockets
epoll
protocol framing
resource limits
signal handling
```

---

# 29. `send()` and `recv()`

TCP:

```c
send(fd, buffer, length, 0);
recv(fd, buffer, length, 0);
```

These have similar partial-I/O considerations to:

```c
write()
read()
```

A successful call does not necessarily mean the entire requested application message was transferred.

---

# 30. `recv()` Returning 0

For a TCP connected socket:

```c
n = recv(fd, buf, size, 0);
```

If:

```text
n == 0
```

it normally means the peer performed an orderly shutdown and there is no more data to read.

Typical server behavior:

```c
if (n == 0)
{
    close(client_fd);
}
```

---

# 31. `send()` Is Not "Delivered to Peer"

Successful:

```c
send()
```

generally means data was accepted by the local networking stack according to the call's semantics.

It does **not** mean:

```text
remote application processed the message
```

The application protocol may need acknowledgements if it requires application-level confirmation.

---

# 32. `shutdown()`

```c
shutdown(fd, SHUT_WR);
```

Half-close the write side.

Possible modes:

```text
SHUT_RD
SHUT_WR
SHUT_RDWR
```

This is different from immediately destroying the entire socket with:

```c
close(fd);
```

---

# 33. TCP Connection Close

Simplified:

```text
Application
   ↓
close()/shutdown()
   ↓
TCP FIN
   ↓
peer ACK
   ↓
peer FIN
   ↓
ACK
```

TCP has multiple states during connection setup and teardown.

Important states include:

```text
LISTEN
SYN_SENT
SYN_RECV
ESTABLISHED
FIN_WAIT_1
FIN_WAIT_2
CLOSE_WAIT
LAST_ACK
TIME_WAIT
```

---

# 34. `TIME_WAIT`

`TIME_WAIT` is a TCP state associated with connection termination.

It helps protect the protocol from delayed old segments and allows proper handling of the final connection termination.

A common misconception:

> `TIME_WAIT` does not simply mean the application forgot to close the socket.

It is part of TCP protocol behavior.

---

# 35. UDP Server

Typical flow:

```text
socket()
   ↓
bind()
   ↓
recvfrom()
   ↓
sendto()
```

No:

```text
listen()
accept()
```

is required.

---

# 36. UDP Server Example

```c
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(void)
{
    int fd = socket(AF_INET,
                    SOCK_DGRAM,
                    0);

    if (fd == -1)
    {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = {0};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd,
             (struct sockaddr *)&addr,
             sizeof(addr)) == -1)
    {
        perror("bind");
        close(fd);
        return 1;
    }

    char buffer[1024];

    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);

    ssize_t n = recvfrom(fd,
                         buffer,
                         sizeof(buffer),
                         0,
                         (struct sockaddr *)&client,
                         &client_len);

    if (n > 0)
    {
        sendto(fd,
               buffer,
               n,
               0,
               (struct sockaddr *)&client,
               client_len);
    }

    close(fd);

    return 0;
}
```

---

# 37. `sendto()` / `recvfrom()`

UDP preserves datagram boundaries.

Example:

```text
sendto(message A)
sendto(message B)
```

The receiver receives datagrams separately.

This differs fundamentally from TCP's byte-stream model.

---

# 38. `connect()` on UDP

UDP can also use:

```c
connect(fd, ...);
```

This does not create a TCP-like reliable connection.

It associates a default peer with the socket and can restrict accepted incoming datagrams to that peer on supported semantics.

Then applications can use:

```c
send()
recv()
```

instead of:

```c
sendto()
recvfrom()
```

---

# 39. DNS and `getaddrinfo()`

Do not hard-code IPv4-only assumptions in modern network programs.

Use:

```c
getaddrinfo()
```

It can resolve:

```text
hostname
service
```

and provide suitable address structures.

Typical pattern:

```text
getaddrinfo()
    ↓
iterate results
    ↓
socket()
    ↓
connect()
```

---

# 40. `getaddrinfo()` Example

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>

int main(void)
{
    struct addrinfo hints = {0};
    struct addrinfo *result;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int ret = getaddrinfo("example.com",
                          "80",
                          &hints,
                          &result);

    if (ret != 0)
    {
        fprintf(stderr,
                "getaddrinfo: %s\n",
                gai_strerror(ret));

        return 1;
    }

    for (struct addrinfo *p = result;
         p != NULL;
         p = p->ai_next)
    {
        printf("family=%d socktype=%d\n",
               p->ai_family,
               p->ai_socktype);
    }

    freeaddrinfo(result);

    return 0;
}
```

---

# 41. Socket Options

Use:

```c
setsockopt()
getsockopt()
```

Common options:

```text
SO_REUSEADDR
SO_REUSEPORT
SO_RCVBUF
SO_SNDBUF
SO_KEEPALIVE
TCP_NODELAY
SO_RCVTIMEO
SO_SNDTIMEO
```

The available options depend on socket family/protocol.

---

# 42. `SO_REUSEADDR`

Common server pattern:

```c
int opt = 1;

setsockopt(fd,
           SOL_SOCKET,
           SO_REUSEADDR,
           &opt,
           sizeof(opt));
```

It can make restarting servers easier in certain address/reuse situations.

Do not interpret it as:

```text
"allow anything to bind to the same port"
```

Its exact behavior depends on address state and platform semantics.

---

# 43. `TCP_NODELAY`

TCP can use Nagle's algorithm to reduce small-packet overhead.

For latency-sensitive applications:

```c
setsockopt(fd,
           IPPROTO_TCP,
           TCP_NODELAY,
           &opt,
           sizeof(opt));
```

may disable Nagle's algorithm.

Trade-off:

```text
lower small-message latency
vs
potentially more packets
```

---

# 44. Socket Buffers

Sockets have kernel-managed buffering.

Conceptually:

```text
Application
     |
     v
socket
+-------------+
| send buffer |
| recv buffer |
+-------------+
     |
     v
TCP/IP stack
```

Applications do not directly control every packet transmission.

The kernel manages protocol state and buffering.

---

# 45. TCP Flow Control

TCP uses a receive window to prevent a fast sender from overwhelming the receiver.

Conceptually:

```text
Sender
  |
  | data
  v
Network
  |
  v
Receiver
  |
  v
receive buffer
```

If the receiver cannot consume data fast enough:

```text
advertised receive window decreases
```

This is **flow control**.

---

# 46. TCP Congestion Control

Congestion control deals with the network path.

Simplified:

```text
Sender
  ↓
Network
  ↓
Routers
  ↓
Receiver
```

If congestion is detected:

```text
sender reduces sending behavior
```

Important distinction:

```text
Flow control
 → protects receiver

Congestion control
 → responds to network congestion
```

---

# 47. TCP Reliability

TCP uses:

```text
sequence numbers
ACKs
retransmission
timers
checksums
```

Simplified:

```text
Sender
  |
  | seq=100
  v
Receiver
  |
  | ACK
  v
Sender
```

If data is lost:

```text
timeout / duplicate ACK related mechanisms
       ↓
retransmission
```

---

# 48. TCP State Machine

Important states:

```text
CLOSED
LISTEN
SYN_SENT
SYN_RECV
ESTABLISHED
FIN_WAIT_1
FIN_WAIT_2
CLOSE_WAIT
CLOSING
LAST_ACK
TIME_WAIT
```

Typical server:

```text
LISTEN
   ↓
SYN_RECV
   ↓
ESTABLISHED
```

Typical client:

```text
CLOSED
  ↓
SYN_SENT
  ↓
ESTABLISHED
```

---

# 49. Linux Socket = File Descriptor

One of the most important Linux concepts:

```text
socket()
    ↓
returns FD
```

Therefore:

```text
read()
write()
close()
fcntl()
poll()
epoll()
```

can operate on sockets where appropriate.

The socket is represented by kernel state behind the descriptor.

---

# 50. Socket FD Mental Model

```text
Process
   |
   v
FD table
   |
   v
struct file
   |
   v
socket-related kernel state
   |
   v
TCP/UDP
```

This connects:

```text
Chapter 11 Files + I/O
```

with:

```text
Chapter 12 Networking
```

---

# 51. Blocking Socket

Default sockets are generally blocking.

Example:

```c
recv(fd, buf, sizeof(buf), 0);
```

If data is unavailable:

```text
process may sleep
```

Conceptually:

```text
recv()
 ↓
no data
 ↓
wait
 ↓
scheduler runs another task
 ↓
packet arrives
 ↓
task wakes
 ↓
recv() returns
```

---

# 52. Non-Blocking Socket

Set:

```c
fcntl(fd, F_SETFL, flags | O_NONBLOCK);
```

Then:

```c
recv()
```

can return immediately if no data is available.

Common result:

```text
-1
errno = EAGAIN/EWOULDBLOCK
```

This is commonly combined with:

```text
epoll
```

---

# 53. `fcntl()` Non-Blocking Example

```c
#include <fcntl.h>

int flags = fcntl(fd, F_GETFL);

if (flags == -1)
{
    perror("fcntl");
}

if (fcntl(fd,
          F_SETFL,
          flags | O_NONBLOCK) == -1)
{
    perror("fcntl");
}
```

---

# 54. `epoll` Networking

Typical high-performance Linux server:

```text
socket()
   ↓
bind()
   ↓
listen()
   ↓
non-blocking
   ↓
epoll_create1()
   ↓
epoll_ctl()
   ↓
epoll_wait()
```

For each ready FD:

```text
accept()
recv()
send()
```

---

# 55. `epoll` Server Architecture

```text
                 epoll
                   |
       +-----------+-----------+
       |           |           |
      fd3         fd4         fd5
       |           |           |
    client A    client B    client C
```

One event loop can manage many connections.

---

# 56. Important `epoll` Pattern

For a non-blocking socket:

```text
EPOLLIN
   ↓
accept/read
   ↓
repeat
   ↓
EAGAIN
   ↓
return to epoll_wait()
```

For writes:

```text
EPOLLOUT
   ↓
send pending data
   ↓
repeat
   ↓
EAGAIN
```

Applications should maintain output buffers for data that could not be fully sent.

---

# 57. Level-Triggered `epoll`

Default behavior is commonly level-triggered.

If data remains available:

```text
EPOLLIN
```

can continue to be reported.

Example:

```text
receive buffer:
1000 bytes

application reads:
100 bytes

remaining:
900 bytes

epoll_wait()
→ readable again
```

---

# 58. Edge-Triggered `epoll`

With:

```c
EPOLLET
```

events are delivered in an edge-triggered manner.

Use:

```text
non-blocking FD
+
drain until EAGAIN
```

Common pattern:

```c
while (1)
{
    n = recv(fd, buf, sizeof(buf), 0);

    if (n > 0)
        process(buf, n);
    else if (n == 0)
        break;
    else if (errno == EAGAIN ||
             errno == EWOULDBLOCK)
        break;
    else
        break;
}
```

---

# 59. `accept()` and `EAGAIN`

For a non-blocking listening socket:

```c
int client = accept(server_fd, NULL, NULL);
```

can return:

```text
-1
EAGAIN/EWOULDBLOCK
```

This means there are currently no connections available to accept.

It is not necessarily a fatal error.

---

# 60. Partial `send()`

Suppose:

```c
send(fd, buffer, 10000, 0);
```

It may return:

```text
4000
```

The remaining:

```text
6000
```

must be retained and sent later.

This is particularly important for non-blocking servers.

---

# 61. Application-Level Send Buffer

A server may maintain:

```text
Connection
+-----------------------+
| input buffer          |
| output buffer         |
| protocol state        |
+-----------------------+
```

When:

```text
send() < pending bytes
```

keep the remaining bytes:

```text
output buffer
```

and wait for:

```text
EPOLLOUT
```

---

# 62. Unix Domain Sockets

Unix domain sockets provide local IPC.

Address family:

```c
AF_UNIX
```

or:

```c
AF_LOCAL
```

Common types:

```text
SOCK_STREAM
SOCK_DGRAM
```

They can be faster than TCP for local communication because they do not need the full IP network path.

---

# 63. Unix Domain Socket Uses

Examples:

```text
local services
database connections
container/runtime communication
desktop IPC
privileged helper communication
```

Address example:

```text
/tmp/my_socket
```

---

# 64. Network Namespace

Linux network namespaces provide isolated networking state.

A namespace can have its own:

```text
interfaces
routes
iptables/nftables state
socket namespace
ports
```

Conceptually:

```text
Host
 |
 +-- namespace A
 |     eth0
 |     routes
 |     sockets
 |
 +-- namespace B
       eth0
       routes
       sockets
```

This is fundamental to Linux containers.

---

# 65. Linux Kernel Network Architecture

High-level:

```text
User Application
       |
       v
Socket API
       |
       v
Socket Layer
       |
       v
Transport Layer
(TCP / UDP)
       |
       v
Network Layer
(IP)
       |
       v
Link Layer
       |
       v
qdisc / device
       |
       v
NIC Driver
       |
       v
NIC
```

---

# 66. Kernel Socket Layer

The socket layer provides a common interface between:

```text
user-space socket APIs
```

and:

```text
protocol implementations
```

Conceptually:

```text
socket()
   ↓
socket layer
   ↓
TCP/UDP
```

This allows applications to use a common API while protocol-specific behavior is implemented underneath.

---

# 67. `struct socket`

Linux internally represents socket-layer state using structures including:

```text
struct socket
```

It connects the generic socket abstraction with protocol-specific state.

A simplified mental model:

```text
struct file
     |
     v
struct socket
     |
     v
protocol-specific state
```

Do not treat this as an exact complete structure layout; kernel structures evolve.

---

# 68. `struct sock`

Protocol implementations use:

```text
struct sock
```

and protocol-specific structures built around it.

Conceptually:

```text
socket layer
     |
     v
struct sock
     |
     +-- TCP state
     +-- buffers
     +-- timers
     +-- protocol state
```

For TCP, additional TCP-specific state exists.

---

# 69. `sk_buff`

A key Linux networking structure is:

```text
struct sk_buff
```

commonly called:

```text
skb
```

It represents a packet buffer and associated metadata as it moves through the networking stack.

Conceptually:

```text
NIC
 ↓
skb
 ↓
network stack
 ↓
TCP/IP
 ↓
socket
```

An skb is more than just a raw packet byte array; it carries metadata and references describing packet data and state.

---

# 70. Receive Path — High Level

A simplified receive path:

```text
NIC
 ↓
DMA into memory
 ↓
NIC driver
 ↓
NAPI
 ↓
skb
 ↓
network stack
 ↓
Ethernet processing
 ↓
IP
 ↓
TCP/UDP
 ↓
socket receive queue
 ↓
recv()
 ↓
application
```

This is one of the most important Linux kernel networking flows.

---

# 71. Transmit Path — High Level

```text
Application
    ↓
send()/write()
    ↓
socket layer
    ↓
TCP/UDP
    ↓
IP
    ↓
routing
    ↓
qdisc
    ↓
NIC driver
    ↓
DMA
    ↓
NIC
    ↓
Network
```

---

# 72. NIC Driver

The network driver interfaces Linux with the physical NIC.

Responsibilities can include:

```text
device initialization
TX/RX queue setup
DMA setup
interrupt handling
NAPI polling
packet transmission
packet reception
statistics
link state
```

---

# 73. Interrupts and NAPI

Traditional model:

```text
packet arrives
    ↓
interrupt
    ↓
CPU handles packet
```

High packet rates can cause excessive interrupt overhead.

Linux uses **NAPI** to combine interrupt notification with polling.

Simplified:

```text
packet arrives
    ↓
interrupt
    ↓
disable/limit further RX interrupts
    ↓
schedule NAPI poll
    ↓
process batch of packets
    ↓
re-enable interrupts when appropriate
```

Batching improves efficiency.

---

# 74. DMA Receive Path

Simplified:

```text
NIC
 |
 | DMA
 v
RAM
 |
 v
driver
 |
 v
skb
 |
 v
network stack
```

The NIC can place packet data into memory using DMA.

The CPU then processes descriptors and packet metadata/data as required.

---

# 75. Routing

When an IP packet needs to leave the host, Linux determines where it should go using routing information.

Conceptually:

```text
destination IP
      ↓
routing lookup
      ↓
output interface
      ↓
next hop
```

Inspect routes:

```bash
ip route
```

Example concept:

```text
default via 192.168.1.1 dev eth0
```

---

# 76. Network Interface Commands

List interfaces:

```bash
ip link
```

Show addresses:

```bash
ip addr
```

Show routes:

```bash
ip route
```

Show sockets:

```bash
ss -tulnp
```

Ping:

```bash
ping 8.8.8.8
```

Trace route:

```bash
traceroute example.com
```

Capture packets:

```bash
tcpdump -i eth0
```

---

# 77. `ss`

Useful for socket debugging:

```bash
ss -tulnp
```

Meaning:

```text
-t → TCP
-u → UDP
-l → listening
-n → numeric
-p → process information
```

For TCP states:

```bash
ss -tan
```

---

# 78. `tcpdump`

Capture traffic:

```bash
sudo tcpdump -i eth0
```

TCP port:

```bash
sudo tcpdump -i eth0 tcp port 8080
```

This helps correlate:

```text
application
+
kernel networking
+
actual packets
```

---

# 79. `strace` Networking

Trace socket-related system calls:

```bash
strace -f \
  -e trace=network \
  ./server
```

You can observe:

```text
socket()
bind()
listen()
accept()
connect()
sendto()
recvfrom()
setsockopt()
```

This is extremely useful during debugging.

---

# 80. Network Debugging Flow

When a server cannot be reached:

```text
1. Is process running?
2. Is socket created?
3. Is bind successful?
4. Is process listening?
5. Is correct address/port used?
6. Check ss
7. Check routing
8. Check firewall
9. Check tcpdump
10. Check application protocol
```

Commands:

```bash
ps
ss -ltnp
ip addr
ip route
tcpdump
```

---

# 81. `connect()` Failure Diagnosis

Common errors:

```text
ECONNREFUSED
ETIMEDOUT
ENETUNREACH
EHOSTUNREACH
```

Conceptually:

```text
ECONNREFUSED
→ reachable host but no listener / connection refused

ETIMEDOUT
→ no response within timeout conditions

ENETUNREACH
→ no suitable network route

EHOSTUNREACH
→ host unreachable
```

Always inspect `errno` instead of guessing.

---

# 82. TCP Server: Process-per-Connection

Simple design:

```text
accept()
   ↓
fork()
   ↓
child handles client
```

Architecture:

```text
Parent
  |
  +-- client 1 → child
  |
  +-- client 2 → child
  |
  +-- client 3 → child
```

Advantages:

```text
simple isolation
easy programming model
```

Disadvantages:

```text
process overhead
memory overhead
context-switch overhead
scalability concerns
```

---

# 83. TCP Server: Thread-per-Connection

```text
accept()
   ↓
pthread_create()
   ↓
thread handles client
```

Advantages:

```text
simpler shared-memory programming
less address-space overhead than processes
```

Disadvantages:

```text
many threads
synchronization
stack memory
scheduler overhead
```

---

# 84. Event-Driven Server

Common Linux high-scale model:

```text
one/few event-loop threads
          |
          v
        epoll
          |
   +------+------+------+
   |      |      |      |
 client client client client
```

Advantages:

```text
fewer threads
efficient connection management
good scalability for I/O-bound workloads
```

Disadvantages:

```text
more complex state machines
must avoid blocking operations
partial I/O must be handled
```

---

# 85. Common Server Models

```text
                 Server
                   |
       +-----------+-----------+
       |           |           |
 process/thread   event      hybrid
 per client       driven
```

Hybrid designs may use:

```text
epoll
+
thread pool
```

for CPU-heavy work.

---

# 86. Why Not One Thread Per Connection?

Suppose:

```text
100,000 connections
```

Creating:

```text
100,000 threads
```

can introduce:

```text
large memory consumption
scheduler overhead
context switching
synchronization complexity
```

Event-driven I/O can keep many mostly-idle connections in a small number of threads.

---

# 87. Kernel Scheduling and Networking

A network server involves scheduling:

```text
packet arrives
   ↓
kernel processing
   ↓
socket becomes readable
   ↓
waiting task wakes
   ↓
scheduler selects task
   ↓
application calls recv()
```

Thus:

```text
Networking
+
Interrupt/NAPI
+
Scheduler
+
Wait queues
+
Memory management
```

are interconnected.

---

# 88. Socket Receive Buffer

Conceptually:

```text
Network
   ↓
TCP
   ↓
socket receive buffer
   ↓
recv()
   ↓
application
```

If the application is too slow:

```text
receive buffer fills
```

TCP flow-control behavior can then reduce the sender's effective rate.

---

# 89. Socket Send Buffer

Conceptually:

```text
Application
   ↓
send()
   ↓
socket send buffer
   ↓
TCP
   ↓
network
```

If the send buffer is full:

```text
blocking socket
→ send may block

non-blocking socket
→ send may return EAGAIN/EWOULDBLOCK
```

---

# 90. Backpressure

Backpressure occurs when downstream processing cannot keep up.

Example:

```text
fast producer
     ↓
socket send buffer
     ↓
network
     ↓
slow receiver
```

Eventually:

```text
buffers fill
   ↓
sender slows/blocks
```

A robust system must design for backpressure.

---

# 91. Keepalive

TCP keepalive can detect some cases where a peer becomes unreachable without a normal connection close.

Enable:

```c
int opt = 1;

setsockopt(fd,
           SOL_SOCKET,
           SO_KEEPALIVE,
           &opt,
           sizeof(opt));
```

Keepalive is not a replacement for application-level heartbeat/protocol timeout design.

---

# 92. Application Timeout

A network application should usually consider:

```text
connect timeout
read timeout
write timeout
idle timeout
request timeout
```

Never assume the network will always respond.

Non-blocking I/O plus:

```text
epoll
timer
```

is a common Linux design.

---

# 93. `SIGPIPE`

Writing to a socket whose peer has closed can potentially result in `SIGPIPE` depending on the operation/platform behavior.

Applications may handle it with:

```c
signal(SIGPIPE, SIG_IGN);
```

or use appropriate socket flags where supported, such as:

```text
MSG_NOSIGNAL
```

when calling `send()`.

Always design explicit error handling around socket writes.

---

# 94. Network Byte Ordering Example

Wrong:

```c
addr.sin_port = 8080;
```

Correct:

```c
addr.sin_port = htons(8080);
```

For IPv4 addresses:

```c
inet_pton(AF_INET,
          "127.0.0.1",
          &addr.sin_addr);
```

Avoid manually manipulating network byte order unless you understand the representation.

---

# 95. Important Socket APIs

Memorize this sequence:

```text
socket()
bind()
listen()
accept()
connect()
send()
recv()
sendto()
recvfrom()
setsockopt()
getsockopt()
shutdown()
close()
```

Also know:

```text
getaddrinfo()
fcntl()
poll()
select()
epoll_create1()
epoll_ctl()
epoll_wait()
```

---

# 96. TCP Server Interview Flow

If asked:

> Explain a TCP server.

Answer:

```text
socket()
  ↓
bind()
  ↓
listen()
  ↓
accept()
  ↓
connected socket
  ↓
recv()/read()
  ↓
process request
  ↓
send()/write()
  ↓
close()/shutdown()
```

The listening FD and connected client FD are different.

---

# 97. What Happens During `accept()`?

Simplified:

```text
client sends connection request
       ↓
TCP handshake processing
       ↓
connection becomes established
       ↓
connection available to listener
       ↓
accept()
       ↓
new connected socket FD
```

The listening socket remains available for additional connections.

---

# 98. What Happens During `connect()`?

Simplified:

```text
application
   ↓
connect()
   ↓
TCP connection establishment
   ↓
SYN
   ↓
SYN-ACK
   ↓
ACK
   ↓
ESTABLISHED
   ↓
connect() returns
```

For blocking sockets, `connect()` may wait.

For non-blocking sockets, connection establishment may complete asynchronously.

---

# 99. Non-Blocking `connect()`

Typical pattern:

```text
socket()
 ↓
O_NONBLOCK
 ↓
connect()
 ↓
EINPROGRESS
 ↓
epoll/poll
 ↓
socket writable
 ↓
getsockopt(SO_ERROR)
 ↓
connection success/failure
```

Important:

> Writable notification alone should not be treated as unconditional connection success; check `SO_ERROR`.

---

# 100. Network Kernel Receive Path — Detailed Mental Model

```text
             HARDWARE
                 |
                 v
              NIC RX
                 |
                DMA
                 |
                 v
              RAM
                 |
                 v
          NIC driver/NAPI
                 |
                 v
                skb
                 |
                 v
           Ethernet layer
                 |
                 v
              IP layer
                 |
                 v
          TCP / UDP layer
                 |
                 v
          socket receive queue
                 |
                 v
             recv()
                 |
                 v
             userspace
```

This is a core senior Linux interview diagram.

---

# 101. Network Kernel Transmit Path — Detailed Mental Model

```text
userspace
   |
   v
send()/write()
   |
   v
socket layer
   |
   v
TCP/UDP
   |
   v
IP
   |
   v
routing
   |
   v
qdisc
   |
   v
NIC driver
   |
   v
DMA
   |
   v
NIC
   |
   v
network
```

---

# 102. Where Does `sk_buff` Fit?

Simplified:

```text
NIC driver
    |
    v
  skb
    |
    v
network stack
    |
    +--> Ethernet
    +--> IP
    +--> TCP/UDP
    |
    v
socket
```

For transmit:

```text
socket/protocol
    ↓
skb
    ↓
routing/qdisc/driver
    ↓
NIC
```

The exact skb lifecycle can vary significantly depending on path and optimization.

---

# 103. NAPI Interview Answer

If asked:

> Why does Linux use NAPI?

Answer:

```text
At high packet rates, one interrupt per packet can create
large CPU overhead.

NAPI combines interrupt notification with polling/batching.

A packet arrival triggers/schedules processing, and the driver
then processes multiple packets in a poll cycle.

This reduces interrupt overhead and improves throughput.
```

---

# 104. Network Namespaces

Important for:

```text
containers
network isolation
virtual networking
```

Conceptually:

```text
Network namespace A
   |
   +-- interfaces
   +-- routes
   +-- sockets

Network namespace B
   |
   +-- interfaces
   +-- routes
   +-- sockets
```

Each namespace has its own networking context.

---

# 105. Virtual Ethernet Pair

Containers commonly use a virtual Ethernet pair:

```text
namespace A
   |
  veth
   |
   |
  veth
   |
namespace B / host bridge
```

A packet can travel:

```text
container
 ↓
veth
 ↓
bridge
 ↓
host networking
 ↓
physical NIC
```

This is a key container-networking concept.

---

# 106. Bridge

A Linux bridge operates at the link layer.

Conceptually:

```text
             bridge
          /     |      \
        veth   veth    NIC
         |      |
      container container
```

The bridge forwards Ethernet frames based on learned MAC addresses.

---

# 107. Routing vs Bridging

### Bridge

```text
Layer 2
MAC-based forwarding
```

### Routing

```text
Layer 3
IP-based forwarding
```

Think:

```text
Bridge → Ethernet frames
Router → IP packets
```

---

# 108. Firewall Position

Linux packet filtering can interact with packets at various networking-stack points.

Modern Linux commonly uses:

```text
nftables
```

and the kernel networking stack provides packet filtering hooks.

For debugging, remember:

```text
application
 ↓
socket/network stack
 ↓
firewall/filtering/routing points
 ↓
NIC
```

Exact packet traversal depends on ingress/egress path and configuration.

---

# 109. Network Security Groups vs Host Firewall

For Linux host-level understanding:

```text
application
 ↓
Linux network stack
 ↓
host firewall
 ↓
NIC
```

In cloud environments, additional filtering may exist outside the host:

```text
cloud network
 ↓
security controls
 ↓
host
 ↓
application
```

Do not assume a local application error is always an application bug.

---

# 110. Important Performance Concepts

Senior Linux networking interviews may discuss:

```text
packet rate
bandwidth
latency
throughput
CPU utilization
context switches
interrupt rate
cache locality
NUMA
DMA
RSS
RPS
XDP
zero-copy
busy polling
```

These should be studied after understanding the basic socket and kernel path.

---

# 111. RSS — Receive Side Scaling

RSS can distribute network receive processing across multiple CPU cores.

Conceptually:

```text
NIC
 |
 +--> RX queue 0 → CPU 0
 |
 +--> RX queue 1 → CPU 1
 |
 +--> RX queue 2 → CPU 2
 |
 +--> RX queue 3 → CPU 3
```

This helps scale packet processing on multicore systems.

---

# 112. XDP — High-Level View

XDP = eXpress Data Path.

It allows packet processing very early in the Linux networking path, typically using eBPF programs attached at the driver/XDP layer.

Conceptually:

```text
NIC
 ↓
XDP
 ↓
network stack
```

Possible uses:

```text
fast filtering
DDoS mitigation
packet processing
high-performance networking
```

It is an advanced topic but useful for senior networking roles.

---

# 113. Zero-Copy Networking

The term "zero-copy" generally means reducing/eliminating particular unnecessary copies between layers.

Examples include:

```text
sendfile()
splice()
mmap()
certain NIC/kernel/application techniques
```

Do not interpret it as:

```text
CPU never touches data
```

or:

```text
there are literally zero memory operations
```

---

# 114. Networking + Memory

Networking is strongly connected to memory management.

Important concepts:

```text
socket buffers
sk_buff
DMA buffers
page allocation
slab/slub allocation
page cache for file/network interactions
copy_to_user()
copy_from_user()
```

A packet path can involve several memory ownership/reference transitions.

---

# 115. Networking + Scheduler

A network application can transition through:

```text
running
   ↓
blocking on recv()
   ↓
sleeping
   ↓
packet arrives
   ↓
socket becomes readable
   ↓
task becomes runnable
   ↓
scheduler
   ↓
application executes
```

For event-driven applications:

```text
epoll_wait()
   ↓
sleep
   ↓
event
   ↓
wake
   ↓
process many ready sockets
```

---

# 116. Common Networking Mistakes

### Mistake 1

Assuming TCP preserves application messages.

Wrong.

TCP is a byte stream.

---

### Mistake 2

Assuming one `recv()` gets one `send()`.

Wrong.

---

### Mistake 3

Assuming `send()` means remote application received data.

Wrong.

---

### Mistake 4

Using blocking sockets inside an event loop.

Can stall the loop.

---

### Mistake 5

Ignoring partial writes.

Can lose application data.

---

### Mistake 6

Treating `epoll` writable notification as connection success.

Check:

```c
getsockopt(fd,
           SOL_SOCKET,
           SO_ERROR,
           ...);
```

---

### Mistake 7

Using edge-triggered epoll but reading only once.

Usually wrong.

Drain until:

```text
EAGAIN/EWOULDBLOCK
```

---

# 117. Senior Interview Exercise 1 — Echo Server

Implement:

```text
TCP echo server
```

Requirements:

```text
socket
bind
listen
accept
recv
send
close
```

Then improve it:

```text
multiple clients
non-blocking
epoll
partial writes
timeouts
```

---

# 118. Senior Interview Exercise 2 — Multi-Client Epoll Server

Architecture:

```text
listen_fd
    |
    v
epoll
    |
    +---- client 1
    +---- client 2
    +---- client 3
    +---- client N
```

Requirements:

```text
non-blocking sockets
accept loop
recv loop
send buffering
EPOLLIN
EPOLLOUT
connection cleanup
```

---

# 119. Senior Interview Exercise 3 — Length-Prefixed Protocol

Implement:

```text
+--------+----------------+
| length | payload        |
+--------+----------------+
  4 bytes   N bytes
```

Handle:

```text
partial header
partial payload
multiple messages in one recv
one message split across many recv calls
```

This is an excellent test of real TCP programming.

---

# 120. Senior Interview Exercise 4 — UDP Server

Implement:

```text
UDP server
```

using:

```text
socket
bind
recvfrom
sendto
```

Then add:

```text
epoll
timeouts
multiple clients
message validation
```

---

# 121. Senior Interview Exercise 5 — Unix Domain Socket

Implement:

```text
server
client
```

using:

```text
AF_UNIX
SOCK_STREAM
```

Understand:

```text
pathname socket
local IPC
FD-based communication
```

---

# 122. Commands to Memorize

```bash
ip addr
ip link
ip route
ss -tulnp
ss -tan
ping
traceroute
tcpdump
lsof
strace
```

For network namespaces:

```bash
ip netns list
```

For interfaces:

```bash
ethtool eth0
```

---

# 123. One-Minute Revision

```text
socket()
    → create socket

bind()
    → assign local address/port

listen()
    → mark TCP socket as listening

accept()
    → create connected socket for client

connect()
    → initiate connection

send()/recv()
    → TCP data transfer

sendto()/recvfrom()
    → UDP datagrams

shutdown()
    → half/full close

close()
    → release FD

TCP
    → reliable ordered byte stream

UDP
    → connectionless datagrams

FD
    → socket represented through process FD

epoll
    → Linux event notification

non-blocking
    → operation returns instead of waiting

sk_buff
    → central Linux packet-buffer structure

NAPI
    → interrupt + polling/batching model

NIC driver
    → kernel ↔ network device

DMA
    → device-memory transfers

network namespace
    → isolated Linux networking context
```

---

# 124. Most Important Interview Diagrams

## TCP server

```text
socket()
   ↓
bind()
   ↓
listen()
   ↓
accept()
   ↓
recv()
   ↓
process
   ↓
send()
   ↓
close()
```

## TCP client

```text
socket()
   ↓
connect()
   ↓
send()
   ↓
recv()
   ↓
close()
```

## Kernel receive path

```text
NIC
 ↓
DMA
 ↓
NIC driver
 ↓
NAPI
 ↓
skb
 ↓
Ethernet
 ↓
IP
 ↓
TCP/UDP
 ↓
socket receive buffer
 ↓
recv()
 ↓
application
```

## Kernel transmit path

```text
application
 ↓
send()
 ↓
socket
 ↓
TCP/UDP
 ↓
IP
 ↓
routing
 ↓
qdisc
 ↓
NIC driver
 ↓
DMA
 ↓
NIC
 ↓
network
```

---

# 125. Final Senior-Level Answer

If asked:

> **"Explain how a packet received by a Linux server reaches the application."**

A strong answer:

```text
The NIC receives the packet and commonly uses DMA to place packet
data into memory. The NIC/driver processing is coordinated through
the Linux networking driver and NAPI. The packet is represented and
processed through networking-stack structures such as sk_buff.

The packet then passes through the link, IP and transport layers.
For TCP, the TCP layer validates and orders the stream and places
available data into the appropriate socket receive state/buffer.

If an application is blocked in recv(), the kernel can wake the
waiting task when data becomes available. If the application uses
epoll, the socket becomes ready and epoll_wait() returns the event.

The application then calls recv()/read(), and data is transferred
according to the socket API semantics into user space.
```

---

# 126. Final Senior-Level Transmit Answer

> **"What happens when an application sends data?"**

```text
Application
   ↓
send()/write()
   ↓
socket layer
   ↓
TCP/UDP
   ↓
IP
   ↓
routing
   ↓
qdisc / output processing
   ↓
NIC driver
   ↓
DMA
   ↓
NIC
   ↓
network
```

For TCP, the kernel also manages:

```text
sequence numbers
ACKs
retransmission
flow control
congestion control
timers
connection state
```

---

# 127. Chapter 12 Complete Mental Model

```text
                         USER SPACE
+------------------------------------------------------+
| Application                                          |
|                                                      |
| socket() connect() send() recv() epoll_wait()        |
+----------------------------+-------------------------+
                             |
                             | system calls
                             v
                         KERNEL SPACE
+------------------------------------------------------+
| Socket Layer                                         |
|       ↓                                              |
| TCP / UDP                                            |
|       ↓                                              |
| IP / Routing                                         |
|       ↓                                              |
| qdisc / networking processing                        |
|       ↓                                              |
| NIC Driver / NAPI                                    |
|       ↓                                              |
| DMA                                                  |
+----------------------------+-------------------------+
                             |
                             v
                            NIC
                             |
                             v
                          Network
```

---

# 128. Chapter 12 — What to Know for Senior Interviews

### Must know very well

```text
TCP vs UDP
TCP byte-stream behavior
socket lifecycle
socket/bind/listen/accept/connect
send/recv
partial I/O
blocking vs non-blocking
select/poll/epoll
LT vs ET epoll
socket buffers
TCP flow control
TCP congestion control
TCP state machine
TIME_WAIT
Unix domain sockets
network byte order
getaddrinfo()
```

### Linux internals

```text
socket layer
struct socket
struct sock
sk_buff
receive path
transmit path
NAPI
NIC driver
DMA
routing
network namespaces
```

### Advanced

```text
RSS
RPS
XDP
zero-copy
io_uring
busy polling
NUMA-aware networking
high-performance packet processing
```

---

# Chapter 12 → Connection to Previous Chapters

Networking now connects several earlier chapters:

```text
Chapter 2  Processes
     ↓
Chapter 3  Threads
     ↓
Chapter 5  Synchronization
     ↓
Chapter 7  Virtual Memory
     ↓
Chapter 10 IPC
     ↓
Chapter 11 Files + I/O
     ↓
Chapter 12 Networking
```

The most important common Linux abstraction is:

```text
                File Descriptor
                      |
        +-------------+-------------+
        |             |             |
      file          pipe          socket
        |             |             |
      VFS            IPC         networking
```

This is why understanding Linux file descriptors, blocking,
non-blocking I/O, wait queues and `epoll` is foundational for
both system programming and Linux networking.
