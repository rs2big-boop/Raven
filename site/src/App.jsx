import React, { useState } from 'react';
import {
  Terminal, Play, Copy, Check, Sparkles, Cpu, Layers, ShieldCheck,
  Zap, Code2, Globe, Server, CheckCircle2, ArrowRight,
  Box, Bug, ChevronRight, FileCode2, BookOpen
} from 'lucide-react';

const CODE_EXAMPLES = {
  web: {
    title: "Web Server & HTML DSL",
    badge: "stdlib/web",
    description: "Write fullstack web services and render rich HTML with Raven's native builder DSL.",
    code: `import "stdlib/web";

let app = new_web_app();

// Serve dynamic HTML with type-safe Element DSL
app.get("/", fn(req, res) {
    let doc = html_doc({"lang": "en"}, [
        head_el({}, [
            tag("title", {}, "Raven Web Server"),
            style_tag(".hero { padding: 40px; font-family: sans-serif; }")
        ]),
        body_el({}, [
            div_el({"class": "hero"}, [
                h1_el({}, "Hello from Native Raven Web!"),
                p_el({}, "Running at native speed with zero external dependencies."),
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
  data_structures: {
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

export default function App() {
  const [activeTab, setActiveTab] = useState('web');
  const [copiedInstall, setCopiedInstall] = useState(false);
  const [copiedCode, setCopiedCode] = useState(false);
  const [isRunning, setIsRunning] = useState(false);
  const [consoleOutput, setConsoleOutput] = useState(CODE_EXAMPLES.web.output);

  const installCmd = "curl -fsSL https://raven-lang.org/install.sh | bash";

  const handleCopyInstall = () => {
    navigator.clipboard.writeText(installCmd);
    setCopiedInstall(true);
    setTimeout(() => setCopiedInstall(false), 2000);
  };

  const handleCopyCode = () => {
    navigator.clipboard.writeText(CODE_EXAMPLES[activeTab].code);
    setCopiedCode(true);
    setTimeout(() => setCopiedCode(false), 2000);
  };

  const handleRunCode = () => {
    setIsRunning(true);
    setConsoleOutput("Compiling to Corvus Bytecode...");
    setTimeout(() => {
      setIsRunning(false);
      setConsoleOutput(CODE_EXAMPLES[activeTab].output);
    }, 450);
  };

  const handleTabChange = (key) => {
    setActiveTab(key);
    setConsoleOutput(CODE_EXAMPLES[key].output);
  };

  return (
    <div className="min-h-screen bg-[#07090e] text-slate-100 flex flex-col selection:bg-purple-600 selection:text-white">
      {/* Background ambient lighting */}
      <div className="fixed inset-0 pointer-events-none overflow-hidden z-0">
        <div className="absolute top-[-10%] left-[20%] w-[600px] h-[600px] bg-purple-600/10 rounded-full blur-[140px]" />
        <div className="absolute top-[40%] right-[10%] w-[500px] h-[500px] bg-cyan-500/10 rounded-full blur-[140px]" />
        <div className="absolute bottom-[-10%] left-[30%] w-[600px] h-[600px] bg-indigo-600/10 rounded-full blur-[140px]" />
      </div>

      {/* Navigation Header */}
      <header className="sticky top-0 z-50 glass-panel border-b border-slate-800/80">
        <div className="max-w-7xl mx-auto px-6 h-18 flex items-center justify-between">
          <div className="flex items-center space-x-3 cursor-pointer">
            <img src="/logo.png" alt="Raven Logo" className="w-9 h-9 rounded-lg shadow-md shadow-purple-900/40" />
            <div className="flex items-baseline space-x-2">
              <span className="font-extrabold text-xl tracking-tight bg-gradient-to-r from-purple-400 via-indigo-200 to-cyan-400 bg-clip-text text-transparent">
                Raven
              </span>
              <span className="text-xs px-2 py-0.5 rounded-full bg-purple-500/20 text-purple-300 font-mono font-medium border border-purple-500/30">
                v1.1.0
              </span>
            </div>
          </div>

          <nav className="hidden md:flex items-center space-x-8 text-sm font-medium text-slate-300">
            <a href="#playground" className="hover:text-cyan-400 transition-colors">Playground</a>
            <a href="#features" className="hover:text-cyan-400 transition-colors">Features</a>
            <a href="#web-framework" className="hover:text-cyan-400 transition-colors">Web Framework</a>
            <a href="#architecture" className="hover:text-cyan-400 transition-colors">Architecture</a>
            <a href="#tooling" className="hover:text-cyan-400 transition-colors">Tools & Debugger</a>
          </nav>

          <div className="flex items-center space-x-4">
            <a
              href="https://github.com"
              target="_blank"
              rel="noreferrer"
              className="p-2 text-slate-400 hover:text-white transition-colors"
              title="GitHub Repository"
            >
              <svg className="w-5 h-5 fill-current" viewBox="0 0 24 24">
                <path fillRule="evenodd" clipRule="evenodd" d="M12 2C6.477 2 2 6.484 2 12.017c0 4.425 2.865 8.18 6.839 9.504.5.092.682-.217.682-.483 0-.237-.008-.868-.013-1.703-2.782.605-3.369-1.343-3.369-1.343-.454-1.158-1.11-1.466-1.11-1.466-.908-.62.069-.608.069-.608 1.003.07 1.53 1.032 1.53 1.032.892 1.53 2.341 1.088 2.91.832.092-.647.35-1.088.636-1.338-2.22-.253-4.555-1.113-4.555-4.951 0-1.093.39-1.988 1.029-2.688-.103-.253-.446-1.272.098-2.65 0 0 .84-.27 2.75 1.026A9.564 9.564 0 0112 6.844c.85.004 1.705.115 2.504.337 1.909-1.296 2.747-1.027 2.747-1.027.546 1.379.202 2.398.1 2.651.64.7 1.028 1.595 1.028 2.688 0 3.848-2.339 4.695-4.566 4.943.359.309.678.92.678 1.855 0 1.338-.012 2.419-.012 2.747 0 .268.18.58.688.482A10.019 10.019 0 0022 12.017C22 6.484 17.522 2 12 2z" />
              </svg>
            </a>
            <a
              href="#install"
              className="px-4 py-2 rounded-lg bg-gradient-to-r from-purple-600 to-indigo-600 hover:from-purple-500 hover:to-indigo-500 text-sm font-semibold shadow-lg shadow-purple-600/25 transition-all flex items-center space-x-2"
            >
              <span>Get Raven</span>
              <ArrowRight className="w-4 h-4" />
            </a>
          </div>
        </div>
      </header>

      {/* Hero Section */}
      <section className="relative z-10 pt-20 pb-16 px-6 max-w-7xl mx-auto text-center">
        <div className="inline-flex items-center space-x-2 px-3.5 py-1.5 rounded-full bg-purple-950/60 border border-purple-500/30 text-purple-300 text-xs font-medium mb-8 backdrop-blur-md">
          <Sparkles className="w-3.5 h-3.5 text-cyan-400" />
          <span>Raven v1.1.0 Released: Native Web Framework & Full VS Code Debugger</span>
        </div>

        <h1 className="text-5xl md:text-7xl font-extrabold tracking-tight max-w-4xl mx-auto leading-[1.12]">
          Modern Simplicity. <br />
          <span className="bg-gradient-to-r from-purple-400 via-indigo-300 to-cyan-400 bg-clip-text text-transparent">
            Native Concurrency.
          </span>
        </h1>

        <p className="mt-6 text-lg md:text-xl text-slate-400 max-w-2xl mx-auto font-normal leading-relaxed">
          Raven combines the ergonomic syntax of Python, real CSP multi-threading inspired by Go, and gradual types with dual execution engines.
        </p>

        {/* Quick Install Bar */}
        <div id="install" className="mt-10 max-w-xl mx-auto">
          <div className="glass-panel p-2 rounded-xl flex items-center justify-between shadow-2xl border border-slate-700/60">
            <div className="flex items-center space-x-3 px-3 overflow-x-auto text-sm font-mono text-slate-300">
              <span className="text-purple-400 select-none">$</span>
              <span className="whitespace-nowrap">{installCmd}</span>
            </div>
            <button
              onClick={handleCopyInstall}
              className="px-4 py-2 rounded-lg bg-slate-800 hover:bg-slate-700 text-xs font-semibold text-white flex items-center space-x-1.5 transition-colors border border-slate-600/50 cursor-pointer"
            >
              {copiedInstall ? (
                <>
                  <Check className="w-4 h-4 text-emerald-400" />
                  <span className="text-emerald-400">Copied!</span>
                </>
              ) : (
                <>
                  <Copy className="w-4 h-4 text-slate-300" />
                  <span>Copy</span>
                </>
              )}
            </button>
          </div>
          <p className="text-xs text-slate-500 mt-2.5">
            Works out of the box on Linux & macOS. Zero dependencies. C++20 core engine.
          </p>
        </div>
      </section>

      {/* Interactive Code Playground */}
      <section id="playground" className="relative z-10 py-12 px-6 max-w-7xl mx-auto w-full">
        <div className="glass-card rounded-2xl overflow-hidden border border-slate-800 shadow-2xl">
          {/* Playground Top Bar */}
          <div className="px-5 py-3.5 bg-slate-900/90 border-b border-slate-800 flex flex-wrap items-center justify-between gap-4">
            <div className="flex items-center space-x-2 overflow-x-auto">
              {Object.keys(CODE_EXAMPLES).map((key) => {
                const item = CODE_EXAMPLES[key];
                const isActive = activeTab === key;
                return (
                  <button
                    key={key}
                    onClick={() => handleTabChange(key)}
                    className={`px-3.5 py-1.5 rounded-lg text-xs font-semibold transition-all flex items-center space-x-2 cursor-pointer ${
                      isActive
                        ? 'bg-purple-600 text-white shadow-md shadow-purple-600/30'
                        : 'text-slate-400 hover:text-white hover:bg-slate-800'
                    }`}
                  >
                    <span>{item.title}</span>
                    <span className={`text-[10px] px-1.5 py-0.2 rounded font-mono ${isActive ? 'bg-purple-800 text-purple-200' : 'bg-slate-800 text-slate-400'}`}>
                      {item.badge}
                    </span>
                  </button>
                );
              })}
            </div>

            <div className="flex items-center space-x-3">
              <button
                onClick={handleCopyCode}
                className="px-3 py-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-xs text-slate-300 flex items-center space-x-1.5 transition-colors cursor-pointer"
              >
                {copiedCode ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                <span>{copiedCode ? "Copied" : "Copy"}</span>
              </button>
              <button
                onClick={handleRunCode}
                disabled={isRunning}
                className="px-4 py-1.5 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-xs font-bold text-white flex items-center space-x-1.5 transition-all shadow-md shadow-emerald-600/20 cursor-pointer disabled:opacity-50"
              >
                <Play className={`w-3.5 h-3.5 fill-current ${isRunning ? 'animate-spin' : ''}`} />
                <span>{isRunning ? "Running..." : "Run Code"}</span>
              </button>
            </div>
          </div>

          {/* Playground Editor + Terminal split */}
          <div className="grid grid-cols-1 lg:grid-cols-12 min-h-[460px]">
            {/* Code Editor Panel */}
            <div className="lg:col-span-7 bg-[#0b0e17] p-5 font-mono text-sm leading-relaxed overflow-x-auto border-b lg:border-b-0 lg:border-r border-slate-800 flex flex-col justify-between">
              <div>
                <div className="text-xs text-slate-500 mb-3 select-none flex items-center justify-between">
                  <span>main.rvn</span>
                  <span>UTF-8 • Raven 1.1</span>
                </div>
                <pre className="text-slate-200 text-[13px] leading-6 font-mono whitespace-pre">
                  {CODE_EXAMPLES[activeTab].code}
                </pre>
              </div>
              <div className="mt-4 pt-3 border-t border-slate-800/80 text-xs text-slate-500">
                {CODE_EXAMPLES[activeTab].description}
              </div>
            </div>

            {/* Output Console Panel */}
            <div className="lg:col-span-5 bg-[#06080d] p-5 flex flex-col justify-between font-mono">
              <div>
                <div className="flex items-center justify-between mb-3 border-b border-slate-800/80 pb-2">
                  <div className="flex items-center space-x-2 text-xs text-slate-400">
                    <Terminal className="w-3.5 h-3.5 text-purple-400" />
                    <span>Corvus VM Output</span>
                  </div>
                  <span className="text-[10px] text-emerald-400 font-semibold bg-emerald-950/60 px-2 py-0.5 rounded border border-emerald-500/30">
                    Exit code: 0
                  </span>
                </div>
                <pre className="text-slate-300 text-xs leading-5 whitespace-pre-wrap font-mono">
                  {consoleOutput}
                </pre>
              </div>

              <div className="mt-6 text-[11px] text-slate-500 flex items-center justify-between pt-3 border-t border-slate-800/60">
                <span>Memory: 1.4 MB</span>
                <span>VM Cycles: 84 opcodes</span>
              </div>
            </div>
          </div>
        </div>
      </section>

      {/* Features Grid */}
      <section id="features" className="relative z-10 py-20 px-6 max-w-7xl mx-auto">
        <div className="text-center mb-16">
          <h2 className="text-3xl md:text-4xl font-extrabold tracking-tight">
            Engineered for Modern Software
          </h2>
          <p className="mt-3 text-slate-400 max-w-xl mx-auto text-sm">
            Everything you need for microservices, web apps, tools, and background workloads without installing 50 dependencies.
          </p>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
          <div className="glass-panel p-6 rounded-xl hover:border-purple-500/50 transition-all group">
            <div className="w-10 h-10 rounded-lg bg-purple-500/10 border border-purple-500/30 flex items-center justify-center text-purple-400 mb-4 group-hover:scale-105 transition-transform">
              <Cpu className="w-5 h-5" />
            </div>
            <h3 className="text-lg font-bold text-white mb-2">Dual Execution Engine</h3>
            <p className="text-sm text-slate-400 leading-relaxed">
              Instant Tree-Walk Interpreter for REPL and prototyping, paired with the Corvus Bytecode VM and bytecode serializer for maximum throughput.
            </p>
          </div>

          <div className="glass-panel p-6 rounded-xl hover:border-cyan-500/50 transition-all group">
            <div className="w-10 h-10 rounded-lg bg-cyan-500/10 border border-cyan-500/30 flex items-center justify-center text-cyan-400 mb-4 group-hover:scale-105 transition-transform">
              <Layers className="w-5 h-5" />
            </div>
            <h3 className="text-lg font-bold text-white mb-2">Real CSP Multi-threading</h3>
            <p className="text-sm text-slate-400 leading-relaxed">
              Native OS threads, typed channels with buffering, WaitGroups, and worker pools. True parallelism without the limitations of a GIL.
            </p>
          </div>

          <div id="web-framework" className="glass-panel p-6 rounded-xl hover:border-emerald-500/50 transition-all group">
            <div className="w-10 h-10 rounded-lg bg-emerald-500/10 border border-emerald-500/30 flex items-center justify-center text-emerald-400 mb-4 group-hover:scale-105 transition-transform">
              <Globe className="w-5 h-5" />
            </div>
            <h3 className="text-lg font-bold text-white mb-2">Native Web Framework</h3>
            <p className="text-sm text-slate-400 leading-relaxed">
              Built-in <code className="text-emerald-300 font-mono text-xs">stdlib/web</code> with Express-style routing, HTML Element Builder DSL, and string templating out of the box.
            </p>
          </div>

          <div className="glass-panel p-6 rounded-xl hover:border-amber-500/50 transition-all group">
            <div className="w-10 h-10 rounded-lg bg-amber-500/10 border border-amber-500/30 flex items-center justify-center text-amber-400 mb-4 group-hover:scale-105 transition-transform">
              <ShieldCheck className="w-5 h-5" />
            </div>
            <h3 className="text-lg font-bold text-white mb-2">Gradual Type Annotations</h3>
            <p className="text-sm text-slate-400 leading-relaxed">
              Type-safe when building critical systems (<code className="text-amber-300 font-mono text-xs">let id: Number</code>), dynamic when scripting. Verified both semantically and at runtime.
            </p>
          </div>

          <div id="tooling" className="glass-panel p-6 rounded-xl hover:border-indigo-500/50 transition-all group">
            <div className="w-10 h-10 rounded-lg bg-indigo-500/10 border border-indigo-500/30 flex items-center justify-center text-indigo-400 mb-4 group-hover:scale-105 transition-transform">
              <Bug className="w-5 h-5" />
            </div>
            <h3 className="text-lg font-bold text-white mb-2">rvndb & VS Code Debugger</h3>
            <p className="text-sm text-slate-400 leading-relaxed">
              First-class debugging with gutter breakpoints, F5 integration, call stack inspection, and expression evaluation right inside VS Code.
            </p>
          </div>

          <div className="glass-panel p-6 rounded-xl hover:border-pink-500/50 transition-all group">
            <div className="w-10 h-10 rounded-lg bg-pink-500/10 border border-pink-500/30 flex items-center justify-center text-pink-400 mb-4 group-hover:scale-105 transition-transform">
              <Box className="w-5 h-5" />
            </div>
            <h3 className="text-lg font-bold text-white mb-2">Production Standard Library</h3>
            <p className="text-sm text-slate-400 leading-relaxed">
              Comprehensive collections (Trie, BST, Deque, Heap), crypto (HMAC-SHA256), TCP/UDP sockets, JSON, CSV, Regex, and package manager.
            </p>
          </div>
        </div>
      </section>

      {/* Architecture Section */}
      <section id="architecture" className="relative z-10 py-16 px-6 max-w-7xl mx-auto border-t border-slate-800/80">
        <div className="text-center mb-12">
          <h2 className="text-3xl font-bold tracking-tight">Clean Modular Architecture</h2>
          <p className="mt-2 text-slate-400 text-sm">
            High-cohesion C++20 engine designed with rigorous separation between frontend, IR, runtime, and VM.
          </p>
        </div>

        <div className="glass-card p-6 md:p-8 rounded-2xl border border-slate-800">
          <div className="grid grid-cols-1 md:grid-cols-4 gap-4 text-center text-xs">
            <div className="p-4 rounded-xl bg-slate-900/80 border border-slate-700/60">
              <div className="font-bold text-purple-400 mb-1 text-sm">1. Frontend</div>
              <p className="text-slate-400">Lexer • Tokenizer • Parser • AST • Diagnostics</p>
            </div>
            <div className="p-4 rounded-xl bg-slate-900/80 border border-slate-700/60">
              <div className="font-bold text-indigo-400 mb-1 text-sm">2. Semantic & IR</div>
              <p className="text-slate-400">Resolver • Symbol Table • Type Checker • IR Builder</p>
            </div>
            <div className="p-4 rounded-xl bg-slate-900/80 border border-slate-700/60">
              <div className="font-bold text-cyan-400 mb-1 text-sm">3. Execution</div>
              <p className="text-slate-400">Tree-Walk Interpreter • Corvus Register/Stack VM</p>
            </div>
            <div className="p-4 rounded-xl bg-slate-900/80 border border-slate-700/60">
              <div className="font-bold text-emerald-400 mb-1 text-sm">4. Runtimes</div>
              <p className="text-slate-400">Managed Heap • Cycle GC • CSP Schedulers • POSIX Sockets</p>
            </div>
          </div>
        </div>
      </section>

      {/* Footer */}
      <footer className="mt-auto border-t border-slate-800/80 py-12 px-6 bg-[#05070a]">
        <div className="max-w-7xl mx-auto flex flex-col md:flex-row items-center justify-between gap-6 text-xs text-slate-500">
          <div className="flex items-center space-x-3">
            <img src="/logo.png" alt="Raven Logo" className="w-6 h-6 rounded" />
            <span className="font-semibold text-slate-300">The Raven Programming Language</span>
            <span>• MIT License</span>
          </div>
          <div className="flex items-center space-x-6">
            <a href="#playground" className="hover:text-slate-300 transition-colors">Playground</a>
            <a href="#features" className="hover:text-slate-300 transition-colors">Features</a>
            <a href="#web-framework" className="hover:text-slate-300 transition-colors">Web Framework</a>
            <a href="#tooling" className="hover:text-slate-300 transition-colors">Debugger</a>
            <a href="https://github.com" className="hover:text-slate-300 transition-colors">GitHub</a>
          </div>
        </div>
      </footer>
    </div>
  );
}
