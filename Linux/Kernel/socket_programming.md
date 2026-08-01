# Socket Programming Interview Notes (Linux/C/C++)

---

# 1. What is a Socket?

A **socket** is an endpoint for communication between two processes over a network.

```
Application
      │
Socket API
      │
TCP / UDP
      │
IP
      │
Network
```

Think of it as a **file descriptor for network communication**.

---

# 2. Types of Sockets

| Type | Protocol | Reliable | Connection |
|-------|----------|----------|------------|
| SOCK_STREAM | TCP | Yes | Connection-oriented |
| SOCK_DGRAM | UDP | No | Connectionless |
| SOCK_RAW | Raw IP | N/A | Packet-level access |

---

# 3. Socket Creation

```c
int sockfd = socket(AF_INET, SOCK_STREAM, 0);
```

Parameters

- AF_INET → IPv4
- SOCK_STREAM → TCP
- 0 → Default protocol (TCP)

Returns

- Socket FD
- -1 on error

---

# 4. TCP Client Flow

```
socket()

↓

connect()

↓

send()/recv()

↓

close()
```

---

# 5. TCP Server Flow

```
socket()

↓

bind()

↓

listen()

↓

accept()

↓

send()/recv()

↓

close()
```

---

# 6. bind()

Associates socket with an IP and Port.

```c
bind(sockfd,
     (struct sockaddr *)&addr,
     sizeof(addr));
```

Without bind()

- Server cannot receive client requests.

---

# 7. listen()

Marks socket as passive.

```c
listen(sockfd, 5);
```

5 = Backlog Queue

---

# 8. accept()

Waits for incoming client.

```c
int client =
accept(sockfd,
       NULL,
       NULL);
```

Returns

New socket.

Important

```
Listening Socket

↓

accept()

↓

New Connected Socket
```

Listening socket continues accepting clients.

---

# 9. connect()

Used by client.

```c
connect(sockfd,
        (struct sockaddr *)&server,
        sizeof(server));
```

Performs TCP Three-way Handshake.

---

# 10. send()

```c
send(sockfd,
     buffer,
     len,
     0);
```

Returns

Number of bytes sent.

---

# 11. recv()

```c
recv(sockfd,
     buffer,
     sizeof(buffer),
     0);
```

Returns

- Bytes received
- 0 → Peer closed connection
- -1 → Error

---

# 12. close()

```c
close(sockfd);
```

Releases socket.

---

# 13. sockaddr_in

```c
struct sockaddr_in {

    short sin_family;

    unsigned short sin_port;

    struct in_addr sin_addr;

};
```

Example

```c
struct sockaddr_in server;

server.sin_family = AF_INET;

server.sin_port = htons(8080);

inet_pton(AF_INET,
          "127.0.0.1",
          &server.sin_addr);
```

---

# 14. htons()

Host to Network Short

```
Little Endian

↓

Big Endian
```

Network always uses Big Endian.

Functions

```
htons()

htonl()

ntohs()

ntohl()
```

---

# 15. inet_pton()

Converts

```
"192.168.1.1"

↓

Binary IP
```

---

# 16. TCP Communication

```
Client

send()

↓

TCP

↓

Server

recv()
```

Reliable

Ordered

Guaranteed Delivery

---

# 17. UDP Communication

```
sendto()

↓

Network

↓

recvfrom()
```

No connection.

No guarantee.

---

# 18. Blocking Socket

Default.

```
recv()

↓

Waits until data arrives
```

Simple but can block forever.

---

# 19. Non-blocking Socket

```c
fcntl(sockfd,
      F_SETFL,
      O_NONBLOCK);
```

recv()

Returns immediately.

---

# 20. select()

Monitor multiple sockets.

```
Socket1

Socket2

Socket3

↓

select()

↓

Ready Socket
```

Limitation

FD_SETSIZE (~1024).

---

# 21. poll()

Improves over select().

No bitmap limitation.

Still O(n).

---

# 22. epoll()

Linux high-performance API.

```
Thousands of sockets

↓

epoll_wait()

↓

Only Ready Events
```

Complexity

O(Ready Events)

Preferred for servers.

---

# 23. Socket Options

```c
setsockopt()
```

Common

SO_REUSEADDR

SO_KEEPALIVE

SO_RCVBUF

SO_SNDBUF

TCP_NODELAY

---

# 24. SO_REUSEADDR

Allows immediate reuse of port.

Useful after restarting server.

---

# 25. SO_KEEPALIVE

Detects dead peers.

Avoids hanging connections.

---

# 26. TCP_NODELAY

Disables Nagle Algorithm.

Small packets sent immediately.

Lower latency.

---

# 27. TCP Connection States

```
LISTEN

↓

SYN_RECEIVED

↓

ESTABLISHED

↓

FIN_WAIT

↓

TIME_WAIT

↓

CLOSED
```

---

# 28. TIME_WAIT

Purpose

Delayed packets disappear.

Connection safely closes.

---

# 29. Partial send()

```
send()

↓

1000 Bytes

↓

Returns

400
```

Need loop.

---

# 30. Partial recv()

```
recv()

↓

May receive

100 Bytes

↓

Need loop
```

TCP is stream-oriented.

---

# 31. SIGPIPE

Writing to closed socket.

Default

Program terminates.

Avoid

```
MSG_NOSIGNAL

or

signal(SIGPIPE, SIG_IGN)
```

---

# 32. Common Errors

```
EAGAIN

EWOULDBLOCK

ECONNRESET

ETIMEDOUT

EPIPE

ECONNREFUSED
```

---

# 33. Thread-per-Connection

```
Client1

↓

Thread1

Client2

↓

Thread2
```

Easy

Poor scalability.

---

# 34. Event-driven Server

```
10000 Clients

↓

epoll

↓

Few Threads
```

Highly scalable.

---

# 35. Interview Questions

### Basic

- What is a socket?
- Why is socket a file descriptor?
- Difference between TCP and UDP?
- What does bind() do?
- Why listen()?
- Why accept() returns new socket?

### Intermediate

- Why htons()?
- Why partial send()?
- Blocking vs Non-blocking?
- select vs poll vs epoll?
- What is backlog queue?
- Why TIME_WAIT?

### Advanced

- Design scalable TCP server.
- How does epoll work?
- Level-triggered vs Edge-triggered epoll?
- How to avoid thundering herd?
- How to handle 1 million connections?
- Why use SO_REUSEADDR?
- Why use TCP_NODELAY?

---

# Complete TCP Server Flow

```
socket()

↓

setsockopt()

↓

bind()

↓

listen()

↓

accept()

↓

recv()

↓

process()

↓

send()

↓

close(client)

↓

accept(next client)
```

---

# Complete TCP Client Flow

```
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

---

# Quick Revision

- Socket = Network file descriptor
- TCP = Reliable, Connection-oriented
- UDP = Fast, Connectionless
- socket() → Create socket
- bind() → Attach IP + Port
- listen() → Start listening
- accept() → New client socket
- connect() → Establish connection
- send()/recv() → Data transfer
- close() → Close socket
- select() = Small servers
- poll() = Better than select()
- epoll() = Best for Linux high concurrency
- SO_REUSEADDR = Reuse port
- TCP_NODELAY = Disable Nagle
- SO_KEEPALIVE = Detect dead peers
- Partial send()/recv() must be handled
- TCP is byte-stream, not message-oriented