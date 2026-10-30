# 🐦 Learn Raven in 15 Minutes

Welcome to **Raven**, a modern, expressive, and high-performance programming language combining the ergonomic simplicity of Python, Go, and Swift with the speed of a dual-engine architecture (Tree-Walk Interpreter + Corvus Bytecode VM).

---

## 1. Hello, World!

Raven syntax is clean and friendly. Semicolons are optional, and `print` accepts multiple arguments separated by spaces.

```raven
print("Hello, Raven!")
print("Version:", 1.0, "Engine: Corvus")
```

---

## 2. Variables & Constants

Define variables with `let`. Raven supports numbers, strings, booleans, arrays, maps, and `nil`.

```raven
let name = "Ada"
let age = 36
let is_programmer = true
let scores = [98, 95, 100]
let user = {"id": 1, "username": "ada_lovelace"}

// Multi-line strings
let banner = """
    ======================
       Raven v1.0.0 CLI
    ======================
"""
```

---

## 3. Control Flow

Parentheses around conditions are **optional**. Blocks are delimited with `{ }`.

### If / Else
```raven
let score = 85

if score >= 90 {
    print("Grade: A")
} else if score >= 80 {
    print("Grade: B")
} else {
    print("Grade: C")
}
```

### Loops: For-In & While
```raven
// Iterate over arrays
let fruits = ["apple", "banana", "cherry"]
for fruit in fruits {
    print("Fruit:", fruit)
}

// While loop
let count = 3
while count > 0 {
    print("Countdown:", count)
    count -= 1
}

// Infinite loop with break
loop {
    print("Loop once")
    break
}
```

---

## 4. Functions & Closures

Declare functions with `fn`. Default parameters and variadic arguments are supported.

```raven
// Default parameter
fn greet(name = "World") {
    return "Hello, " + name + "!"
}
print(greet())           // "Hello, World!"
print(greet("Raven"))    // "Hello, Raven!"

// Variadic functions (...args)
fn sum_all(...numbers) {
    let total = 0
    for n in numbers {
        total += n
    }
    return total
}
print("Sum:", sum_all(1, 2, 3, 4, 5)) // Sum: 15

// Anonymous functions (Lambdas)
let square = fn(x) { return x * x }
print("Square of 4:", square(4))
```

---

## 5. Modern Operators

Raven includes first-class modern operators:

```raven
// 1. Null-coalescing (??)
let display_name = user_input ?? "Guest"

// 2. Optional chaining (?.)
let city = profile?.address?.city

// 3. Runtime Type Checking (is)
if age is Number {
    print("Valid numeric age")
}

// 4. Spread Operator (...)
let base = [1, 2]
let extended = [...base, 3, 4] // [1, 2, 3, 4]

// 5. Destructuring
let [first, second] = [10, 20]
let {id, username} = user
```

---

## 6. Built-in Data Structures (`stdlib/collections`)

Raven comes with a rich standard library for data structures:

```raven
import "stdlib/collections"

// Stack (LIFO)
let stack = create_stack()
stack.push("A")
stack.push("B")
print(stack.pop()) // "B"

// Queue (FIFO)
let queue = create_queue()
queue.enqueue("First")
queue.enqueue("Second")
print(queue.dequeue()) // "First"

// Deque (Double-Ended)
let deque = create_deque()
deque.push_front(1)
deque.push_back(2)

// Priority Queue (Min-Heap)
let pq = create_priority_queue()
pq.push("Important task", 1)
pq.push("Background task", 10)
print(pq.pop()) // "Important task"

// Binary Search Tree (BST)
let bst = create_bst()
bst.insert(42)
bst.insert(10)
print("Contains 10:", bst.contains(10))

// Prefix Tree (Trie)
let trie = create_trie()
trie.insert("raven")
print("Has prefix 'rav':", trie.starts_with("rav"))
```

---

## 7. Networking & HTTP

Build web servers and clients effortlessly:

```raven
import "stdlib/net/http"

// HTTP Server
let server = new_http_server()

server.get("/", fn(req, res) {
    res.json({"status": "ok", "language": "Raven"})
})

server.get("/greet/:name", fn(req, res) {
    res.send("Hello, " + req.params["name"])
})

// server.listen("127.0.0.1", 8080)
```

---

## 8. Summary & Next Steps

Raven is designed to be:
1. **Easy to learn**: Intuitive syntax, clean errors with "Did you mean?" suggestions.
2. **Batteries-included**: Sockets, HTTP, HMAC-SHA256, SQLite, Collections, Concurrency.
3. **Flexible**: Use semicolons and parentheses if you prefer C-style, or omit them for modern clean code.

Run your scripts anytime with:
```bash
raven run script.rvn
# Or enter interactive REPL
raven
```
