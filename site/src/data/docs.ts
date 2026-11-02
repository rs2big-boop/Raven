export interface DocItem {
  id: string;
  title: string;
  content: string;
}

export interface DocSection {
  id: string;
  title: string;
  items: DocItem[];
}

export const DOCS_SECTIONS: DocSection[] = [
  {
    id: "getting-started",
    title: "Getting Started",
    items: [
      {
        id: "install",
        title: "Installation",
        content: `Raven can be installed with a single command on Linux and macOS:

\`\`\`bash
curl -fsSL https://raven-lang.org/install.sh | bash
\`\`\`

Verify your installation:

\`\`\`bash
raven --version
# Output: Raven 1.1.0 (Corvus Engine)
\`\`\`

You can also run Raven scripts directly:
\`\`\`bash
raven script.rvn
\`\`\`
Or compile to bytecode and run with the Corvus VM:
\`\`\`bash
ravenc script.rvn -o script.rvc
rvm script.rvc
\`\`\``
      },
      {
        id: "tour",
        title: "Language in 5 Minutes",
        content: `Raven offers clean, ergonomic syntax with optional semicolons and optional condition parentheses:

\`\`\`raven
// Variables and gradual typing
let greeting = "Hello"
let count: Number = 42

// Functions with default arguments
fn add(a, b = 10): Number {
    return a + b
}

// Flow control
if count > 40 {
    print(greeting, "World!", count)
}

// While loop
let i = 0
while i < 3 {
    print("Iteration:", i)
    i += 1
}
\`\`\``
      }
    ]
  },
  {
    id: "syntax",
    title: "Syntax & Language Features",
    items: [
      {
        id: "types-slices",
        title: "Gradual Types & Slices",
        content: `### Gradual Typing
Raven supports optional type annotations on variable declarations and functions:
\`\`\`raven
let name: String = "Alice"
let age: Number = 30
let active: Bool = true
let items: Array = [1, 2, 3]
let user: Map = {"role": "admin"}
\`\`\`

### Native Slicing
Arrays and strings can be sliced using pythonic \`[start:end]\` syntax:
\`\`\`raven
let numbers = [10, 20, 30, 40, 50]
let sub = numbers[1:4] // [20, 30, 40]
let head = numbers[:2]  // [10, 20]
let tail = numbers[3:]  // [40, 50]

let str_val = "Raven"
print(str_val[0:4])     // "Rave"
\`\`\``
      },
      {
        id: "destructuring-matching",
        title: "Destructuring & Match",
        content: `### Destructuring
Unpack arrays and maps directly into variables:
\`\`\`raven
let [x, y] = [100, 200]
let {"id": uid, "name": uname} = {"id": 1, "name": "Raven"}
\`\`\`

### Pattern Matching
Exhaustive pattern matching with wildcard fallback:
\`\`\`raven
let status = 200
let msg = match (status) {
    200 => "OK",
    404 => "Not Found",
    500 => "Server Error",
    _   => "Unknown"
}
\`\`\``
      }
    ]
  },
  {
    id: "concurrency",
    title: "CSP Concurrency",
    items: [
      {
        id: "threads-channels",
        title: "Threads & Channels",
        content: `Raven implements real OS-level multi-threading inspired by Go's Communicating Sequential Processes (CSP).

\`\`\`raven
import "stdlib/sync";

// Create a buffered channel
let ch = chan_create(10);
let wg = create_wait_group();

wg.add(2);

// Worker 1
spawn(fn() {
    chan_send(ch, "Data from Thread A");
    wg.done();
});

// Worker 2
spawn(fn() {
    chan_send(ch, "Data from Thread B");
    wg.done();
});

wg.wait();
chan_close(ch);

let val = chan_recv(ch);
while (val != nil) {
    print(val);
    val = chan_recv(ch);
}
\`\`\``
      }
    ]
  },
  {
    id: "web",
    title: "Native Web Framework",
    items: [
      {
        id: "web-server",
        title: "HTML DSL & Server",
        content: `Raven includes a native web framework and HTML DSL in \`stdlib/web\`:

\`\`\`raven
import "stdlib/web";

let app = new_web_app();

// Dynamic route with HTML DSL
app.get("/", fn(req, res) {
    let page = html_doc({"lang": "en"}, [
        head_el({}, [tag("title", {}, "Raven App")]),
        body_el({}, [
            div_el({"class": "card"}, [
                h1_el({}, "Welcome to Raven!"),
                p_el({}, "Built-in web routing with zero external npm/pip packages.")
            ])
        ])
    ]);
    res.html(page);
});

// API endpoint
app.get("/api/user", fn(req, res) {
    res.json({"id": 1, "username": "raven_user"});
});

// Serve static assets
app.static("/static", "public");

app.listen(8080);
\`\`\``
      }
    ]
  }
];
