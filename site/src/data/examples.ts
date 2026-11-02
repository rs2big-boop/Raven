export interface CodeExample {
  id: string;
  title: string;
  badge: string;
  description: string;
  code: string;
  output: string;
}

export const CODE_EXAMPLES: Record<string, CodeExample> = {
  web: {
    id: "web",
    title: "Web Server & HTML DSL",
    badge: "stdlib/web",
    description: "Write fullstack web applications and render rich HTML with Raven's native builder DSL.",
    code: `import "stdlib/web";

let app = new_web_app();

// Dynamic route with HTML DSL
app.get("/", fn(req, res) {
    let doc = html_doc({"lang": "en"}, [
        head_el({}, [
            tag("title", {}, "Raven Powered Web"),
            style_tag(".hero { padding: 40px; font-family: sans-serif; }")
        ]),
        body_el({}, [
            div_el({"class": "hero"}, [
                h1_el({}, "Hello from Native Raven Web!"),
                p_el({}, "Fast, concurrent and ergonomic."),
                a_el({"href": "/api/stats", "class": "btn"}, "View JSON Metrics")
            ])
        ])
    ]);
    res.html(doc);
});

// JSON API Route
app.get("/api/stats", fn(req, res) {
    res.json({
        "status": "online",
        "engine": "Corvus VM v1.1.0",
        "active_threads": 4
    });
});

app.listen(8080);`,
    output: `🐦 Raven Web Server listening on http://127.0.0.1:8080
[HTTP] GET / -> 200 OK (text/html; charset=utf-8) [0.42ms]
[HTTP] GET /api/stats -> 200 OK (application/json) [0.18ms]`
  },
  concurrency: {
    id: "concurrency",
    title: "CSP Concurrency & Channels",
    badge: "Multi-threaded",
    description: "Spawn real OS threads communicating over lock-free channels with wait groups.",
    code: `import "stdlib/sync";

let ch = chan_create(10);
let wg = create_wait_group();

// Spawn 3 concurrent background workers
let worker_id = 1;
while (worker_id <= 3) {
    let id = worker_id;
    wg.add(1);
    spawn(fn() {
        let result = id * 100;
        chan_send(ch, "Worker #" + str(id) + " calculated: " + str(result));
        wg.done();
    });
    worker_id += 1;
}

// Collector thread
wg.wait();
chan_close(ch);

let msg = chan_recv(ch);
while (msg != nil) {
    print("Received: " + msg);
    msg = chan_recv(ch);
}`,
    output: `Received: Worker #1 calculated: 100
Received: Worker #2 calculated: 200
Received: Worker #3 calculated: 300
[Execution completed in 1.1ms across 3 OS threads]`
  },
  types: {
    id: "types",
    title: "Gradual Typing & Slicing",
    badge: "Modern Syntax",
    description: "Opt-in type safety with zero boilerplate and pythonic slicing syntax.",
    code: `// Gradual type annotations with runtime & compile validation
let service_name: String = "Corvus Engine"
let port: Number = 8080
let is_secure: Bool = true

// Native array and string slicing
let numbers = [10, 20, 30, 40, 50, 60]
let slice_middle = numbers[1:4]   // [20, 30, 40]
let head = numbers[:3]            // [10, 20, 30]
let tail = numbers[3:]            // [40, 50, 60]

let text = "Raven Programming Language"
let word = text[0:5]              // "Raven"

print(service_name + " on port " + str(port))
print("Middle slice: " + str(slice_middle))
print("Extracted word: " + word)`,
    output: `Corvus Engine on port 8080
Middle slice: [20, 30, 40]
Extracted word: Raven`
  },
  collections: {
    id: "collections",
    title: "Comprehensive Data Structures",
    badge: "stdlib/collections",
    description: "Built-in Deque, Min-Heap PriorityQueue, Trie, BST, and RingBuffer.",
    code: `import "stdlib/collections";

// Min-Heap Priority Queue
let pq = create_priority_queue();
pq.push("Process Background Job", 10);
pq.push("Send Critical Alert", 1);
pq.push("Flush Cache", 5);

print("Highest priority task: " + pq.pop());
print("Next task: " + pq.pop());

// Prefix Tree (Trie)
let trie = create_trie();
trie.insert("raven");
trie.insert("race");
trie.insert("radar");

print("Words with prefix 'ra': " + str(trie.words_with_prefix("ra")));`,
    output: `Highest priority task: Send Critical Alert
Next task: Flush Cache
Words with prefix 'ra': ["race", "radar", "raven"]`
  },
  matching: {
    id: "matching",
    title: "Pattern Matching & Destructuring",
    badge: "Ergonomics",
    description: "Clean destructuring of arrays & maps with exhaustive pattern matching.",
    code: `// Array and Map Destructuring
let coords = [1920, 1080]
let [width, height] = coords

let user = {"id": 101, "role": "admin", "name": "Ada"}
let {"id": uid, "role": urole} = user

// Pattern Matching
let response_code = 404
let status_text = match (response_code) {
    200 => "OK - Success",
    400 => "Bad Request",
    404 => "Resource Not Found",
    500 => "Internal Server Error",
    _   => "Unknown Status"
}

print("Dimensions: " + str(width) + "x" + str(height))
print("User #" + str(uid) + " (" + urole + "): " + status_text)`,
    output: `Dimensions: 1920x1080
User #101 (admin): Resource Not Found`
  }
};
