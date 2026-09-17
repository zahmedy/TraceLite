# TraceLite

TraceLite is a lightweight Linux debugging agent built with eBPF, C++, and Python.

The goal is to provide useful system-level debugging information with minimal overhead and without requiring heavyweight observability infrastructure.

TraceLite is designed primarily as a learning project for understanding Linux internals, eBPF, system calls, networking, storage, scheduling, and production debugging.

## Goals

TraceLite should:

- Have very low idle overhead.
- Collect information only when needed.
- Perform filtering and aggregation as close to the kernel as possible.
- Avoid continuously sending large volumes of events to userspace.
- Provide simple debugging commands for common Linux problems.
- Remain small enough that the entire system can be understood by one engineer.

This is not intended to replace full observability platforms such as Datadog, Splunk, or New Relic.

The focus is targeted debugging.

## Architecture

```text
                 ┌────────────────────┐
                 │    Python CLI      │
                 │                    │
                 │    tracectl        │
                 └─────────┬──────────┘
                           │
                    Unix domain socket
                           │
                 ┌─────────▼──────────┐
                 │     C++ Agent      │
                 │                    │
                 │ collectors         │
                 │ aggregation        │
                 │ request handling   │
                 └─────────┬──────────┘
                           │
                         libbpf
                           │
                 ┌─────────▼──────────┐
                 │   eBPF Programs    │
                 │                    │
                 │ exec tracing       │
                 │ disk tracing       │
                 │ TCP tracing        │
                 │ scheduler tracing  │
                 └─────────┬──────────┘
                           │
                      Linux kernel
```

## Components

### eBPF programs

The `bpf/` directory contains programs that execute inside the Linux kernel.

These programs observe kernel events such as:

- process execution
- TCP connections
- block device operations
- system call failures
- scheduler events

eBPF programs should remain small.

Their primary responsibilities are:

```text
observe
filter
aggregate
report
```

Expensive processing should happen in userspace.

### C++ agent

The C++ agent loads and manages the eBPF programs using libbpf.

It is responsible for:

- loading eBPF programs
- attaching probes
- reading eBPF maps
- consuming ring buffer events
- maintaining debugging sessions
- communicating with the CLI

The agent runs as a small daemon.

Example:

```text
tracelite-agent
```

### Python CLI

The Python CLI provides the user-facing debugging interface.

Example:

```bash
tracectl exec
```

Future commands may include:

```bash
tracectl disk
tracectl tcp
tracectl syscalls
tracectl cpu
tracectl runqueue
```

The Python process should not directly communicate with the kernel.

Instead:

```text
Python CLI
    ↓
Unix socket
    ↓
C++ agent
    ↓
eBPF
```

## Version 0.1 — Process Execution Tracing

The first TraceLite feature will observe programs executed through `execve()`.

Example:

```bash
tracectl exec
```

Possible output:

```text
LAST 10 SECONDS

COMMAND                         COUNT
/usr/bin/python3                   38
/usr/bin/curl                      17
/usr/bin/sed                       12
/usr/bin/ssh                        4
```

The data path is:

```text
sys_enter_execve
       ↓
eBPF program
       ↓
eBPF map
       ↓
C++ collector
       ↓
Unix socket
       ↓
Python CLI
```

The initial eBPF program will attach to:

```text
tracepoint:syscalls:sys_enter_execve
```

The eBPF program will collect information such as:

```text
PID
parent PID
command
execution count
```

Where possible, counts should be aggregated inside the kernel instead of sending one event for every process execution.

## Project Structure

```text
tracelite/
├── README.md
├── Makefile
├── .gitignore
│
├── bpf/
│   ├── exec.bpf.c
│   └── common.h
│
├── agent/
│   ├── main.cpp
│   ├── exec_collector.cpp
│   ├── exec_collector.h
│   ├── server.cpp
│   └── server.h
│
├── cli/
│   └── tracectl.py
│
├── include/
│   └── protocol.h
│
├── tests/
│   ├── test_cli.py
│   └── exec_test.sh
│
└── scripts/
    └── setup_dev.sh
```

## Planned Features

Future versions may add:

### Process debugging

```bash
tracectl exec
```

Find unexpectedly spawned processes.

### System call errors

```bash
tracectl syscalls
```

Identify frequently failing system calls and error codes.

### Disk latency

```bash
tracectl disk
```

Identify slow block I/O operations and processes generating disk traffic.

### TCP debugging

```bash
tracectl tcp
```

Observe TCP connection attempts, failures, and destinations.

### CPU profiling

```bash
tracectl cpu
```

Determine where CPU time is being spent.

### Scheduler latency

```bash
tracectl runqueue
```

Measure how long processes wait before receiving CPU time.

## Design Principles

### Low overhead

TraceLite should remain nearly invisible when debugging features are inactive.

### Kernel-side aggregation

Prefer:

```text
100,000 kernel events
       ↓
eBPF aggregation
       ↓
20 summarized results
```

instead of:

```text
100,000 kernel events
       ↓
100,000 userspace events
```

### Bounded memory

All maps and buffers must have explicit size limits.

The debugging agent must never become the cause of the system problem being investigated.

### On-demand tracing

Expensive tracing should only be enabled when requested.

Example:

```bash
tracectl disk --duration 10
```

rather than continuously collecting everything.

## Development Strategy

New debugging features should generally follow this workflow:

```text
Linux problem
    ↓
investigate with bpftrace
    ↓
identify useful kernel probe
    ↓
prototype bpftrace program
    ↓
implement eBPF program
    ↓
add C++ collector
    ↓
expose through tracectl
```

For example:

```text
Problem:
"Which programs are constantly being launched?"

Prototype:

tracepoint:syscalls:sys_enter_execve
{
    @[str(args->filename)] = count();
}

Then convert the successful experiment into a permanent TraceLite collector.
```

This keeps development driven by real debugging questions instead of adding probes simply because they exist.

## Status

TraceLite is currently experimental.

Initial milestone:

```text
[ ] Load first eBPF program with libbpf
[ ] Attach to execve tracepoint
[ ] Count executed programs
[ ] Read counts from C++
[ ] Implement Unix domain socket
[ ] Implement Python CLI
[ ] Support `tracectl exec`
```

The first milestone is complete when running:

```bash
tracectl exec
```

provides a useful summary of programs executed on the host.
