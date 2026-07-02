## Ariadne

**Ariadne** is an Express-inspired HTTP framework built from scratch in modern C++ using raw POSIX sockets. Ariadne does not rely on existing networking libraries and instead implements the core pieces of an HTTP server manually. These include : request parsing, routing, middleware execution, response generation, persistent HTTP/1.1 connections and concurrent request processing using a custom thread pool.

The project is the result of curiosity regarding web frameworks and their low level operations in general. The goal was to build an understanding of said low level operations and answer questions like -

- How are HTTP requests parsed in the first place
- How are routes , especially routes with route parameters, matched efficiently?
- How does a thread pool improve concurrency and is that the best way to do it?

Building Ariadne meant implementing these mechanisms myself and also finally uncovering these black boxes that one could easily look over when using frameworks and libraries like Express(Node.js) or net/http(Go).

## Why the name "Ariadne"

In Greek mythology, Ariadne, a Cretan princess, gave the hero Theseus a thread that guided him through the deadly Labyrinth. And to me, the name felt appropriate for an HTTP framework that guides incoming requests to the right handlers. Every request is routed through middleware before reaching its appropriate handler, much like following a thread through a maze.

## Benchmark Overview

After the implementation of the framework, I benchmarked Ariadne against two of the other HTTP framework implementations that I myself am familiar with -

- **net/http of Go** which is Go's standard http server
- **Express** - Node.js with express.

All three servers exposed the exact same API, were containerised and run with identical limits on resources and were run using a standard k6 load testing config.
The purpose of the benchmark was not to try to crown my **Ariadne** the winner (or another framework that might have won) but instead to understand how different architectures in these three servers would behave under a similar I/O bound and CPU-bound loads.

As a result, I uncovered two real architectural issues in Ariadne's implementation which only became apparent under load.
Fixing these issues provided, what I believe to be, the most important insights of the project and are discussed in the sections below.

## Framework Overview

Ariadne is built around a lightweight request processing pipeline inspired by frameworks like Express while implementing each component from scratch using modern C++ and the POSIX sockets API.

```
TCP Connection
      │
      ▼
HTTP Request Parsing
      │
      ▼
Middleware Pipeline
      │
      ▼
Route Matching
      │
      ▼
Route Handler
      │
      ▼
HTTP Response
```

### Features

- Manual HTTP/1.1 request parsing with support for request headers and Content-Length-based body parsing.
- Persistent HTTP/1.1 connections (Keep-Alive) with correct connection lifecycle management.
- Express-inspired routing API supporting GET, POST, PUT, and DELETE handlers.
- Dynamic route matching with parameterized paths (e.g. /users/:id).
- Middleware pipeline with support for request interception and early termination.
- Fluent response API supporting method chaining, custom headers, status codes, and JSON responses.
- Fixed-size thread pool for concurrent request processing using std::thread, std::mutex, and std::condition_variable.
- Graceful shutdown of worker threads and active client connections.

### Example API written in Ariadne

```cpp
Server app;

app.use(loggerMiddleware);

app.get("/", [](Request& req, Response& res) {
    res.send("Hello, World!");
});

app.post("/submit", [](Request& req, Response& res) {
    res.status(201).json(R"({"success":true})");
});

app.listen(3000);
```

### Repo Structure

```txt
ariadne/
├── src/
│   ├── server.cpp        // HTTP server, routing, connection handling, and middleware execution
│   ├── response.cpp      // HTTP response generation and serialization
│   ├── threadpool.cpp    // Worker thread pool for handling concurrent requests
│   ├── main.cpp          // Main entry point for the core framework (if running directly)
│   └── *.h               // Declaration files
│
├── benchmarks/           // Framework performance comparison servers
│   ├── ariadne/          // Ariadne benchmark implementation (C++)
│   │   ├── Dockerfile
│   │   └── main.cpp
│   ├── express/          // Express.js benchmark implementation (Node.js)
│   │   ├── Dockerfile
│   │   ├── package.json
│   │   └── server.js
│   ├── net-http/         // net/http benchmark implementation (Go)
│   │   ├── Dockerfile
│   │   └── main.go
│   ├── results/          // Text files containing the k6 load testing results
│   │   ├── ariadne.txt
│   │   ├── express.txt
│   │   ├── nethttp.txt
│   │   └── *-cpu-load.txt
│   └── loadtest.js       // Default k6 script testing multiple standard endpoints
│
├── examples/             // Sample applications using the Ariadne framework
│   ├── basic.cpp         // Basic setup and routing example
│   ├── json.cpp          // Returning JSON payload example
│   └── status.cpp        // Custom status code responses example
│
└── run.sh                // Bash script to compile and run the framework (uses g++ -std=c++20)
```

## Benchmark Methodology

To ensure a fair comparison, all three servers implemented the exact same API and response semantics.
Docker images of the three servers were benchmarked with identical resource limits which were as follows -

- CPU - 4 cores
- Memory - 512 MB

This ensured that differences in performance would not arise due to unfair benchmarking conditions.

The **workload** implemented using k6 was as follows -

| Endpoint       | Method | Weight | Purpose                                     |
| -------------- | ------ | -----: | ------------------------------------------- |
| `/`            | GET    |    30% | Lightweight text response                   |
| `/api/user`    | GET    |    25% | Small JSON response                         |
| `/submit`      | POST   |    20% | Request body parsing                        |
| `/api/compute` | GET    |    20% | CPU-bound recursive computation (`fib(30)`) |
| `/error`       | GET    |     5% | Error response generation                   |

The distribution was kept as such to more accurately represent a real-world scenario instead of synthetically testing a single endpoint.

No dedicated compute-only benchmark was performed. All CPU bound measurements reported later were obtained using the above mixed workload.

### Load Scenarios

Two independent scenarios were executed using k6 -

**1. Steady Load**
Constant 50 virtual users
Duration: 2 minutes

This measures stable throughput and latency under sustained traffic.

**2. Ramping Load**
Start at 0 virtual users
Ramp to 50 VUs over 30 seconds
Ramp to 200 VUs over 1 minute
Sustain 200 VUs for 30 seconds
Ramp back down to 0 over 30 seconds

This scenario stresses each implementation under rapidly increasing concurrency and is intended to expose scalability bottlenecks that may not appear under steady traffic.

## Phase 1 - Lightweight non-CPU workload

This was the first round of testing done, you can see the k6 results captured in the [benchmarks/results](/benchmarks/results) directory.

### Results

| Server      | Throughput | Overall Avg | Overall p95 | Steady p95 | Ramping p95 |
| ----------- | ---------- | ----------- | ----------- | ---------- | ----------- |
| Express     | 751 req/s  | 2.84ms      | 7.44ms      | 8.61ms     | 6.87ms      |
| Go net/http | 719 req/s  | 6.62ms      | 16.08ms     | 9.72ms     | 16.92ms     |
| Ariadne     | 707 req/s  | 7.75ms      | 18.27ms     | 8.84ms     | 19.34ms     |

_Same k6 script (steady 50 VUs for 2m, ramping 0→200 VUs over 2.5m), same Docker limits (`--cpus=4.0 --memory=512m`) for all three servers._

Although Express achieved the highest throughput in this workload, the difference between all three implementations was relatively small. Since the requests required very little computation, this benchmark measured the overhead of parsing, route dispatching and sending responses.

Express benefited from Node.js' single threaded event loop here. As there was no CPU work required, the requests could be processed without any thread scheduling or synchronisation overhead in Express. Whereas in the case of **net/http** and **Ariadne**, they hand requests to worker threads, introducing a small amount of overhead even for these inexpensive operations.

This behaviour is expected as at this stage the benchmark has no work that can benefit from parallel execution and thus avoiding thread management naturally becomes an advantage.

## HTTP/1.1 Keep-Alive Protocol violation

The initial benchmark results appeared promising until the server was subjected to sustained load. During the ramping scenario, Ariadne consistently exhibited request failures accompanied by messages like "unsolicited response received on idle HTTP channel".
Once I looked around a little, I found out that this was due to a violation of the HTTP/1.1 protocol.

### What was wrong

Under HTTP/1.1 connections are to be persistent by default. Unless either endpoint explicitly requests otherwise, both the client and server assume that the TCP connection will remain open for future requests.

However, Ariadne's original connection handling looked roughly like this:

```txt
accept()
    ↓
parseRequest()
    ↓
executeMiddlewares()
    ↓
sendResponse()
    ↓
close(clientFd)
```

Although the server advertised HTTP/1.1, the server immediately closed the socket after serving a single request.
And k6 correctly assumed that the connection remained usable and returned it to its socket pool. Later when it tries to issue another request over the same socket, the one that Ariadne closed, we see connection resets and aborted requests.

The implementation was effectively speaking HTTP/1.0 connection semantics while claiming to be HTTP/1.1.

### The Fix

Instead of treating a TCP connection as a single request, the server was restructured so that each worker thread owned a connection until either:

the client explicitly requested Connection: close,
the client disconnected, or
a socket error occurred.

The processing model changed from:

```
Connection
    ↓
Request
    ↓
Response
    ↓
Close

```

to

```
Connection
        │
        ▼
Request 1
        ▼
Response 1
        ▼
Request 2
        ▼
Response 2
        ▼
...
        ▼
Connection Closed
```

This required introducing a persistent request-processing loop within each worker thread while correctly respecting the Connection header defined by HTTP/1.1.

In addition to the protocol fix, a number of related robustness improvements were made:

- Added receive timeouts (SO_RCVTIMEO) to prevent indefinitely blocked worker threads.
- Added protection against SIGPIPE when writing to disconnected sockets.
- Wrapped request parsing inside exception handling to prevent malformed requests from terminating worker threads.

**As a result**, after implementing persistent connections, the HTTP protocol violations disappeared entirely. More importantly, Ariadne now behaved consistently with both Go's net/http and Express, making subsequent benchmark comparisons both fairer and significantly more representative of real-world HTTP server behaviour.

## Phase 2 - CPU-bound Workload and Thread Pool starvation

After fixing the keep-alive issue, I added a new CPU-intensive endpoint(the '/api/compute' endpoint mentioned above was added here) which would put net/http and Ariadne's strength to use.

This endpoint recursively calculates `fib(30)`. That is 1.3 million recursive function calls. This endpoint is 20% of all requests meaning the other 80% remain lightweight calls.

### The Unexpected Result

The benchmark results were dramatically worse tan expected.

Throughput collapsed from the hundreds of requests per second to roughly 40 requests per second, and k6 reported widespread timeouts, even for the lightweight routes like '/' and '/api/user'.

At first glance, I thought the recursive computation was the culprit. But it could not have been so because in that case the single threaded event loop would surely fall behind. As it would have to do all those calculations on a single thread as compared to the 4 that I had configured in Ariadne. And even if Ariadne is unoptimised as compared to net/http, single threaded Node.js. And I was right.

### The Root cause and the Fix

The actual problem was introduced by the previous keep-alive fix I had implemented.

Originally, a worker thread processed exactly one request before returning to the thread pool.

After implementing proper HTTP/1.1 persistent connections, each worker instead became responsible for an entire TCP connection.
Ariadne's thread pool size was determined using `std::thread::hardware_concurrency()`. For the benchmark image, I had manually set this to 4 workers for no apparent reason in hindsight.

That meant Ariadne could process **only 4 simultaneous connections** regardless of how lightweight the other requests might be.

But k6 generated up to 200 concurrent virtual users and with only four worker threads available:

- four client connections occupied the worker threads
- the remaining 196 waited in the accept queue.
  and no additional requests could begin processing until an existing connection closed.

The expensive fib(30) endpoint merely exposed this architectural limitation by keeping each connection busy for longer. The fundamental issue was not the recursive computation—it was that the thread pool had been sized for **CPU parallelism** rather than **expected connection concurrency**.

This was an important distinction that I had initially overlooked.

A fixed-size thread pool is often discussed in terms of the number of CPU cores available. However, once each worker owns an entire persistent connection, the pool size effectively becomes the maximum number of simultaneous active client connections possible.

The benchmark highlighted that concurrency (how many connections can make progress) and parallelism (how many tasks execute simultaneously on the CPU) are related, but not interchangeable. My original implementation optimized for the latter while unintentionally limiting the former.

**The fix** was simple I resized the threadpool to use 250 threads which would easily accommodate the 200 concurrent users I was testing against. At first you might think that at 8MB per thread this would mean I'd need 2GB RAM. However that is not the case.

- By default, Linux reserves an 8MB chunk of Virtual Memory for each thread's stack. Docker limits don't care about Virtual Memory.
- Docker's --memory flag only restricts Resident Set Size (Physical RAM). A thread only uses physical RAM for the stack space it actually touches.

This was verified by monitoring the docker stats during the benchmark. Even at 200 concurrent users the total RAM usage by the running container did not cross 15MB.

## Final Benchmark Results: Mixed I/O + CPU-Bound Workload

| Server                | Throughput | Overall Avg | Steady p95 | Ramping p95  | Ramping p99  |
| --------------------- | ---------- | ----------- | ---------- | ------------ | ------------ |
| Ariadne (250 threads) | 756 req/s  | 1.97ms      | 19.41ms    | 3.77ms       | 5.58ms       |
| Go net/http           | 756 req/s  | 2.06ms      | 17.51ms    | 3.78ms       | 6.77ms       |
| Express               | 550 req/s  | 40.11ms     | 16.31ms    | **153.55ms** | **198.85ms** |

_Added `GET /api/compute` (recursive `fib(30)`, ~1.3M calls) to the route mix, weighted 20% of traffic alongside the original I/O-bound routes (`/` 30%, `/api/user` 25%, `/submit` 20%, `/error` 5%). Same k6 scenarios and Docker limits as above. Ariadne's thread pool was resized from 4 threads to 250 (check [here](#the-root-cause-and-the-fix) for why) to match keep-alive's per-connection thread pinning._

Under mixed CPU + I/O load, Ariadne reaches throughput parity with Go net/http once its thread pool is correctly sized — both handle the 200-VU spike cleanly because both can run `fib(30)` calls in true parallel across 4 cores. Express, however, degrades sharply: its single-threaded event loop has to fully serialize every `fib(30)` call, so under the ramping spike, requests queue behind each other on the one thread ramping p95 balloons to 153ms, roughly 40x worse than Go or Ariadne.

This is the inverse of the I/O-bound result above: **the same architectural property that made Express fastest on trivial routes (no thread coordination overhead) becomes its biggest liability once real CPU-bound work enters the mix.**

Rather than demonstrating that one framework is universally superior, the benchmark highlights the trade-offs between three fundamentally different concurrency models. Once Ariadne's protocol and connection-management issues were resolved, its behavior aligned closely with Go under the tested workload, while Express remained highly competitive for predominantly I/O-bound traffic.

## Known limitations

- Ariadne uses a thread-per-active-connection model backed by a fixed size thread pool. While this performs well for the tested workloads, it does not scale to thousands of concurrent connections as efficiently as event-driven (epoll) or M:N scheduled (Goroutines) architectures.
- The benchmark uses a synthetic CPU workload (fib(30)) to isolate concurrency behaviour. Real-world CPU work (JSON serialization, compression, cryptography, etc.) may exhibit different characteristics.
- Only HTTP/1.1 is currently supported. HTTP/2, TLS, WebSockets, and chunked transfer encoding are outside the scope of this project.
- Dynamic route matching currently uses a linear search over registered parameterized routes. This is sufficient for small applications but could be optimized using a trie or radix tree for larger routing tables.

## Wrapping up

This benchmark ended up testing less about which language is fast and more about how three different concurrency models react under two different types of loads and also revealing two crucial fixes in Ariadne that taught me a lot.
