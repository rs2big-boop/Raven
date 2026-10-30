# Concurrency & Multithreading in Raven

Raven provides **Native CSP (Communicating Sequential Processes)** concurrency directly on top of operating system threads.

---

## Core Philosophy: Communication Over Shared State

Rather than sharing memory and managing complex thread locks, Raven promotes passing messages through typed, thread-safe channels:

> *"Do not communicate by sharing memory; instead, share memory by communicating."*

---

## The `stdlib/sync` Primitives

Import the synchronization module:
```raven
import "stdlib/sync";
```

### 1. Spawning Tasks (`go`)
Spawn concurrent tasks executing on independent threads:
```raven
go(fn() {
    print("Running in a background thread!");
});
```

### 2. Channels (`create_channel`)
Thread-safe queues for cross-task communication:
```raven
// Unbuffered channel (capacity 0) or buffered channel (capacity > 0)
let ch = create_channel(5);

// Send message
ch.send("Task payload");

// Receive message (blocking)
let msg = ch.recv();
print("Received: " + msg);

// Receive with timeout in milliseconds
let timed_msg = ch.recv_timeout(100);
if (timed_msg == nil) {
    print("Channel receive timed out");
}

// Close channel
ch.close();
```

### 3. WaitGroups (`create_wait_group`)
Coordinate completion across multiple parallel workers:
```raven
let wg = create_wait_group();

let i = 0;
while (i < 4) {
    wg.add(1);
    go(fn() {
        // Perform work...
        wg.done();
    });
    i = i + 1;
}

// Block until all 4 workers call wg.done()
wg.wait();
print("All tasks finished!");
```

### 4. Mutexes (`create_mutex`)
Thread synchronization when in-place shared state is required:
```raven
let mu = create_mutex();
let shared_counter = 0;

mu.lock();
shared_counter = shared_counter + 1;
mu.unlock();
```
