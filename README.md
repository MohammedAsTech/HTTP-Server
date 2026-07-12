# Multi-Threaded HTTP Server

A from-scratch HTTP/1.1 server written in C++17 using raw POSIX sockets, a fixed-size thread pool, and a request router. No external libraries.

## Features

- TCP accept loop with SO_REUSEADDR and IPv4/IPv6 support
- HTTP/1.1 request parser — method, path, version, headers, body
- Request router with exact and prefix path matching, HEAD method fallback per HTTP/1.1 spec
- Static file serving with MIME type detection for html, css, js, json, txt, png, jpg, gif
- POST body support with Content-Length trimming
- Echo handler — mirrors request body and Content-Type back to caller
- Fixed thread pool (4 workers) for concurrent connections
- Thread-safe logger with latency tracking and ANSI color output
- Graceful shutdown on SIGINT — no zombie threads
- Socket receive timeout via select() — silent visitors do not freeze workers
- Oversized request rejection — requests over 4KB return 400
- Path traversal protection — requests containing .. return 403
- Full error coverage — 400, 403, 404, 500

## Project Structure

```
├── include/          # Header files
│   ├── Handler.h     # Abstract Handler, HelloHandler, NotFoundHandler, EchoHandler, StaticFileHandler
│   ├── HttpParser.h
│   ├── HttpRequest.h
│   ├── HttpResponse.h
│   ├── Logger.h
│   ├── Router.h
│   ├── Server.h
│   └── ThreadPool.h
├── src/              # Implementation files
│   ├── HttpParser.cpp
│   ├── HttpResponse.cpp
│   ├── Router.cpp
│   ├── Server.cpp
│   ├── StaticFileHandler.cpp
│   ├── ThreadPool.cpp
│   └── main.cpp
├── tests/
│   ├── test_main.cpp      # 53 unit tests
│   ├── test_server.sh     # 106 shell tests
│   └── test_leaks.sh      # Valgrind memory leak check
├── www/              # Static files served by the server
└── CMakeLists.txt
```

## Build

```bash
# Using CMake directly
cd cmake-build-debug && make

# Or using the root Makefile wrapper
make
```

## Run

```bash
./cmake-build-debug/my_server 8080
```

## Example Requests

```bash
# Hello handler
curl localhost:8080/

# Static file
curl localhost:8080/index.html

# Echo POST
curl -X POST -d "hello world" localhost:8080/echo

# JSON echo
curl -X POST -H "Content-Type: application/json" -d '{"name":"test"}' localhost:8080/echo

# 404
curl localhost:8080/missing

# 403 path traversal
curl --path-as-is localhost:8080/../etc/passwd
```

## Design Patterns

### Thread Pool
`ThreadPool` maintains a fixed set of 4 worker threads that pull tasks from a shared `std::queue`. `Server::acceptLoop()` submits a task per connection and immediately returns to accepting — decoupling connection acceptance from request handling. Workers coordinate access to the queue via a `std::mutex` and `std::condition_variable`, waking only when work is available. On shutdown, a stop flag is set, all workers are notified, and every thread is joined cleanly.

### Strategy
`Handler` is a pure virtual interface with a single `handle(req, res)` method. Each concrete handler — `HelloHandler`, `StaticFileHandler`, `EchoHandler`, `NotFoundHandler` — is a self-contained strategy that encapsulates one behaviour. The `Router` selects the correct strategy at runtime based on method and path, with no switch statements or conditionals in the dispatch logic. Adding a new route requires only a new `Handler` subclass and one `addRoute()` call.

### Singleton
`Logger` is a singleton — `Logger::instance()` always returns the same object, shared across all worker threads. All writes are protected by an internal `std::mutex` so concurrent log lines never interleave. Handlers and the server core call `Logger::instance().log(...)` directly without needing a reference passed through the call chain.

### Chain of Responsibility
`Router::dispatch()` applies a two-stage matching chain to every incoming request. It first attempts an exact `(method, path)` match against a `std::map`. If that fails, it walks a list of prefix routes in registration order, returning the first match. If nothing matches, the catch-all `NotFoundHandler` is returned. No request ever falls through without a response.

### RAII
All resource lifetimes are managed automatically. Handlers are owned by `std::shared_ptr` — the router holds them alive for as long as routes exist and they are destroyed when the router goes out of scope. `ThreadPool::~ThreadPool()` calls `shutdown()` automatically, ensuring all threads are joined before destruction. `Server::~Server()` closes the listening socket. No `delete`, no `free`, no manual cleanup anywhere in the codebase.

## Architecture

```
main.cpp  →  Server  →  ThreadPool (4 workers)
                      →  Router  →  HelloHandler        GET /
                                 →  StaticFileHandler   GET /* (prefix)
                                 →  EchoHandler         POST /echo
                                 →  NotFoundHandler     catch-all
          →  HttpParser  →  HttpRequest
          →  HttpResponse  →  build()
          →  Logger (singleton, mutex-protected)
```

## Complexity

| Operation          | Complexity | Notes                         |
|--------------------|------------|-------------------------------|
| Accept connection  | O(1)       | Single syscall, hand to pool  |
| Parse request      | O(R)       | R = request size, single pass |
| Route dispatch     | O(log N)   | std::map exact match first    |
| Serve static file  | O(F)       | F = file size, disk read      |
| Thread pool submit | O(1)       | Queue push + notify_one       |
| Logger write       | O(L)       | L = log line, mutex locked    |

## Edge Cases Handled

| Situation                  | Response                  |
|----------------------------|---------------------------|
| Malformed HTTP             | 400 Bad Request           |
| Path traversal (..)        | 403 Forbidden             |
| File not found             | 404 Not Found             |
| Unknown route              | 404 Not Found             |
| Wrong method on route      | 404 Not Found             |
| Request over 4KB           | 400 Bad Request           |
| Handler throws exception   | 500 Internal Server Error |
| Silent visitor             | Closed after 5s timeout   |
| HEAD request               | Falls back to GET handler |
| Ctrl+C                     | Graceful shutdown         |

## Test Results

| Suite            | Tests | Passed | Failed |
|------------------|-------|--------|--------|
| Unit tests       | 53    | 53     | 0      |
| Shell tests      | 106   | 106    | 0      |
| Memory leaks     | —     | 0 leaks| —      |

## Development Note

This project was built with the assistance of Claude (Anthropic) as a learning tool. All architectural decisions, design patterns, and implementation steps were walked through interactively — Claude acted as a senior engineer explaining concepts, reviewing code, and catching bugs, while I wrote, integrated, and debugged the actual code. The goal was to deeply understand how HTTP servers, thread pools, and socket programming work at a low level, not to generate code blindly.

## Notes

This project does not implement TLS, HTTP/2, keep-alive, or chunked transfer encoding. It is a learning project demonstrating raw socket programming, concurrency, and HTTP parsing in C++17.
