export interface StdlibPackage {
  name: string;
  category: string;
  description: string;
  functions: string[];
  snippet: string;
}

export const STDLIB_PACKAGES: StdlibPackage[] = [
  {
    name: "stdlib/web",
    category: "Web & Networking",
    description: "Sinatra/Express-style Web Framework, HTML Element Builder DSL, and string templating.",
    functions: ["new_web_app()", "html_doc()", "tag()", "render_template()", "app.get()", "app.post()", "app.static()", "app.listen()"],
    snippet: `import "stdlib/web";
let app = new_web_app();
app.get("/hello", fn(req, res) { res.send("Hello World!"); });
app.listen(8080);`
  },
  {
    name: "stdlib/sync",
    category: "Concurrency",
    description: "Multi-threaded CSP concurrency primitives: channels, WaitGroups, mutexes, and workers.",
    functions: ["spawn()", "chan_create()", "chan_send()", "chan_recv()", "create_wait_group()", "create_mutex()", "create_once()"],
    snippet: `import "stdlib/sync";
let ch = chan_create(5);
spawn(fn() { chan_send(ch, 42); });
print(chan_recv(ch));`
  },
  {
    name: "stdlib/collections",
    category: "Data Structures",
    description: "Production data structures including Min-Heap PriorityQueue, Trie, Deque, BST, RingBuffer, BitSet.",
    functions: ["create_priority_queue()", "create_trie()", "create_deque()", "create_bst()", "create_ring_buffer()", "create_set()"],
    snippet: `import "stdlib/collections";
let pq = create_priority_queue();
pq.push("task", 1);
let top = pq.pop();`
  },
  {
    name: "stdlib/strings",
    category: "Core Utilities",
    description: "String utilities, padding, trimming, case conversions, regex helpers, and token splitters.",
    functions: ["str_repeat()", "pad_left()", "pad_right()", "str_reverse()", "str_count()", "has_prefix()", "has_suffix()", "split()", "join()"],
    snippet: `import "stdlib/strings";
let s = pad_left("42", 5, "0"); // "00042"`
  },
  {
    name: "stdlib/crypto",
    category: "Security",
    description: "Cryptographic hash functions: RFC 2104 compliant HMAC-SHA256, SHA-256, MD5, and Base64.",
    functions: ["hmac_sha256()", "native_sha256()", "native_md5()", "base64_encode()", "base64_decode()", "hash()"],
    snippet: `import "stdlib/crypto";
let sig = hmac_sha256("secret-key", "payload");`
  },
  {
    name: "stdlib/net",
    category: "Web & Networking",
    description: "Non-blocking POSIX TCP/UDP socket servers, clients, connection pools, and HTTP client.",
    functions: ["tcp_listen()", "tcp_connect()", "udp_bind()", "http_get()", "http_request()"],
    snippet: `import "stdlib/net";
let conn = tcp_connect("127.0.0.1", 8080);
conn.write("PING");`
  },
  {
    name: "stdlib/math",
    category: "Core Utilities",
    description: "Mathematical operations, trigonometry, statistics, random numbers, and numeric helpers.",
    functions: ["abs()", "sqrt()", "pow()", "sin()", "cos()", "clamp()", "gcd()", "lcm()", "rand()", "rand_int()"],
    snippet: `import "stdlib/math";
let val = clamp(15, 0, 10); // 10`
  },
  {
    name: "stdlib/io",
    category: "System",
    description: "File I/O, buffered reading/writing, line iteration, directory traversal, and environment vars.",
    functions: ["read_file()", "write_file()", "append_file()", "read_lines()", "file_exists()", "env()", "exec()"],
    snippet: `import "stdlib/io";
let lines = read_lines("data.txt");`
  }
];
