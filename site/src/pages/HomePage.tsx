import React, { useState } from 'react';
import {
  Copy, Check, ArrowRight, Terminal, Cpu, Box, ShieldCheck,
  Zap, Code2, Globe, Bug, Package, GitBranch, Play
} from 'lucide-react';

interface HomePageProps {
  onNavigate: (page: string) => void;
}

export default function HomePage({ onNavigate }: HomePageProps): React.JSX.Element {
  const [activeTab, setActiveTab] = useState<'build' | 'run' | 'pkg'>('build');
  const [copiedCmd, setCopiedCmd] = useState<string | null>(null);

  const commandSnippets = {
    build: "raven build app.rvn -o app && ./app",
    run: "raven run app.rvn",
    pkg: "raven pkg add raven-router"
  };

  const handleCopy = (key: 'build' | 'run' | 'pkg') => {
    navigator.clipboard.writeText(commandSnippets[key]);
    setCopiedCmd(key);
    setTimeout(() => setCopiedCmd(null), 2000);
  };

  return (
    <div className="space-y-20 pb-24">
      {/* Hero Section */}
      <section className="pt-20 px-6 max-w-7xl mx-auto">
        <div className="space-y-6 max-w-4xl">
          {/* Release Badge */}
          <div className="inline-flex items-center space-x-2.5 px-3 py-1 rounded-md bg-[#161d28] border border-amber-500/30 text-amber-400 text-xs font-mono">
            <span className="w-2 h-2 rounded-full bg-amber-400 animate-pulse" />
            <span>Raven v1.0.0 is Official • 148 Tests Passing • Native Standalone Compiler</span>
          </div>

          <h1 className="text-4xl sm:text-6xl lg:text-7xl font-extrabold tracking-tight text-white leading-[1.08]">
            Simple as Python. <br />
            Concurrent as Go. <br />
            <span className="text-amber-400">Standalone as C.</span>
          </h1>

          <p className="text-base sm:text-lg text-slate-400 max-w-2xl leading-relaxed">
            Raven is a modern general-purpose language with a dual execution engine (Tree-Walk Interpreter & Corvus VM), native CSP concurrency, and a built-in compiler that emits standalone 64-bit ELF executables with <strong className="text-slate-200">zero external C compiler dependencies</strong>.
          </p>

          {/* Quick Terminal Command Bar */}
          <div className="pt-2 max-w-2xl">
            <div className="terminal-window rounded-xl overflow-hidden">
              {/* Tab Header */}
              <div className="flex items-center justify-between px-4 py-2.5 bg-[#0a0e17] border-b border-[#1e2633] text-xs font-mono">
                <div className="flex items-center space-x-2">
                  <div className="w-2.5 h-2.5 rounded-full bg-[#30363d]" />
                  <div className="w-2.5 h-2.5 rounded-full bg-[#30363d]" />
                  <div className="w-2.5 h-2.5 rounded-full bg-[#30363d]" />
                  <span className="text-slate-500 pl-2">raven-cli</span>
                </div>
                <div className="flex items-center space-x-1">
                  <button
                    onClick={() => setActiveTab('build')}
                    className={`px-2.5 py-1 rounded text-[11px] transition-colors cursor-pointer ${
                      activeTab === 'build' ? 'bg-[#1b2330] text-amber-400 font-bold' : 'text-slate-500 hover:text-slate-300'
                    }`}
                  >
                    Native Build (No GCC)
                  </button>
                  <button
                    onClick={() => setActiveTab('run')}
                    className={`px-2.5 py-1 rounded text-[11px] transition-colors cursor-pointer ${
                      activeTab === 'run' ? 'bg-[#1b2330] text-amber-400 font-bold' : 'text-slate-500 hover:text-slate-300'
                    }`}
                  >
                    Instant Run
                  </button>
                  <button
                    onClick={() => setActiveTab('pkg')}
                    className={`px-2.5 py-1 rounded text-[11px] transition-colors cursor-pointer ${
                      activeTab === 'pkg' ? 'bg-[#1b2330] text-amber-400 font-bold' : 'text-slate-500 hover:text-slate-300'
                    }`}
                  >
                    Package Hub
                  </button>
                </div>
              </div>

              {/* Terminal Body */}
              <div className="p-4 flex items-center justify-between text-xs font-mono">
                <div className="flex items-center space-x-2.5 overflow-x-auto text-slate-200">
                  <span className="text-amber-400 select-none">$</span>
                  <span>{commandSnippets[activeTab]}</span>
                </div>
                <button
                  onClick={() => handleCopy(activeTab)}
                  className="px-3 py-1.5 rounded-md bg-[#161b22] hover:bg-[#21262d] text-slate-300 hover:text-white border border-[#30363d] transition-colors flex items-center space-x-1.5 cursor-pointer ml-3 shrink-0"
                >
                  {copiedCmd === activeTab ? (
                    <>
                      <Check className="w-3.5 h-3.5 text-emerald-400" />
                      <span className="text-emerald-400">Copied</span>
                    </>
                  ) : (
                    <>
                      <Copy className="w-3.5 h-3.5" />
                      <span>Copy</span>
                    </>
                  )}
                </button>
              </div>
            </div>
          </div>

          {/* Action CTAs & Badges */}
          <div className="flex flex-wrap items-center gap-3 pt-2">
            <button
              onClick={() => onNavigate('playground')}
              className="px-5 py-2.5 rounded-md bg-amber-500 hover:bg-amber-400 text-xs font-bold text-slate-950 transition-colors flex items-center space-x-2 cursor-pointer shadow-sm"
            >
              <Play className="w-3.5 h-3.5 fill-current" />
              <span>Launch Web Playground</span>
            </button>
            <button
              onClick={() => onNavigate('packages')}
              className="px-5 py-2.5 rounded-md bg-[#0d1117] hover:bg-[#161b22] text-xs font-bold text-slate-200 border border-[#30363d] transition-colors flex items-center space-x-2 cursor-pointer"
            >
              <Package className="w-3.5 h-3.5 text-amber-400" />
              <span>Browse Packages</span>
            </button>
            <button
              onClick={() => onNavigate('docs')}
              className="px-5 py-2.5 rounded-md bg-[#0d1117] hover:bg-[#161b22] text-xs font-medium text-slate-400 hover:text-white border border-[#21262d] transition-colors cursor-pointer"
            >
              <span>Language Specification</span>
            </button>
          </div>
        </div>
      </section>

      {/* Core Architectural Pillars */}
      <section className="px-6 max-w-7xl mx-auto space-y-8">
        <div className="border-b border-[#21262d] pb-4 flex flex-col md:flex-row md:items-end justify-between gap-4">
          <div>
            <span className="text-xs font-mono text-amber-400 font-semibold tracking-wider uppercase">
              Engineered Subsystems
            </span>
            <h2 className="text-2xl sm:text-3xl font-extrabold text-white mt-1">
              Everything in One Cohesive Toolchain
            </h2>
          </div>
          <div className="text-xs font-mono text-slate-500">
            No external compiler • No runtime dependencies • 100% C++20 Core
          </div>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-5">
          {/* Pillar 1: Native Standalone Build */}
          <div 
            onClick={() => onNavigate('architecture')}
            className="dev-card rounded-xl p-6 space-y-3 cursor-pointer"
          >
            <div className="w-9 h-9 rounded-lg bg-amber-500/10 border border-amber-500/30 flex items-center justify-center text-amber-400">
              <Zap className="w-4 h-4" />
            </div>
            <h3 className="text-base font-bold text-white">Standalone Native Binary Generator</h3>
            <p className="text-xs text-slate-400 leading-relaxed">
              Compile <code className="text-amber-300">.rvn</code> into a standalone 64-bit ELF executable (<code className="text-amber-300">raven build</code>) embedding the Corvus engine with zero GCC/Clang dependencies.
            </p>
            <div className="pt-2 text-xs font-mono text-amber-400/90 flex items-center space-x-1">
              <span>View build internals</span>
              <ArrowRight className="w-3 h-3" />
            </div>
          </div>

          {/* Pillar 2: Dual Engine */}
          <div 
            onClick={() => onNavigate('architecture')}
            className="dev-card rounded-xl p-6 space-y-3 cursor-pointer"
          >
            <div className="w-9 h-9 rounded-lg bg-sky-500/10 border border-sky-500/30 flex items-center justify-center text-sky-400">
              <Cpu className="w-4 h-4" />
            </div>
            <h3 className="text-base font-bold text-white">Dual-Engine Runtime</h3>
            <p className="text-xs text-slate-400 leading-relaxed">
              Instant Tree-Walk Interpreter for sub-2ms startup and debugging, paired with the Corvus Bytecode VM for optimized loop throughput.
            </p>
            <div className="pt-2 text-xs font-mono text-sky-400/90 flex items-center space-x-1">
              <span>Inspect architecture</span>
              <ArrowRight className="w-3 h-3" />
            </div>
          </div>

          {/* Pillar 3: CSP Concurrency */}
          <div 
            onClick={() => onNavigate('docs')}
            className="dev-card rounded-xl p-6 space-y-3 cursor-pointer"
          >
            <div className="w-9 h-9 rounded-lg bg-emerald-500/10 border border-emerald-500/30 flex items-center justify-center text-emerald-400">
              <GitBranch className="w-4 h-4" />
            </div>
            <h3 className="text-base font-bold text-white">Native CSP Concurrency</h3>
            <p className="text-xs text-slate-400 leading-relaxed">
              True multi-threaded <code className="text-emerald-300">spawn()</code>, thread-safe typed channels (<code className="text-emerald-300">chan</code>), Mutexes, and WaitGroups without a Global Interpreter Lock (GIL).
            </p>
            <div className="pt-2 text-xs font-mono text-emerald-400/90 flex items-center space-x-1">
              <span>Explore concurrency</span>
              <ArrowRight className="w-3 h-3" />
            </div>
          </div>

          {/* Pillar 4: IDE & Debugger */}
          <div 
            onClick={() => onNavigate('docs')}
            className="dev-card rounded-xl p-6 space-y-3 cursor-pointer"
          >
            <div className="w-9 h-9 rounded-lg bg-purple-500/10 border border-purple-500/30 flex items-center justify-center text-purple-400">
              <Bug className="w-4 h-4" />
            </div>
            <h3 className="text-base font-bold text-white">VS Code & Cursor Debugger</h3>
            <p className="text-xs text-slate-400 leading-relaxed">
              Full F5 debugging, editor gutter breakpoints, variable inspection, step over/into, and official LSP language server integration.
            </p>
            <div className="pt-2 text-xs font-mono text-purple-400/90 flex items-center space-x-1">
              <span>Debugger setup guide</span>
              <ArrowRight className="w-3 h-3" />
            </div>
          </div>

          {/* Pillar 5: Packages & Registry */}
          <div 
            onClick={() => onNavigate('packages')}
            className="dev-card rounded-xl p-6 space-y-3 cursor-pointer"
          >
            <div className="w-9 h-9 rounded-lg bg-amber-500/10 border border-amber-500/30 flex items-center justify-center text-amber-400">
              <Package className="w-4 h-4" />
            </div>
            <h3 className="text-base font-bold text-white">Package Registry Hub</h3>
            <p className="text-xs text-slate-400 leading-relaxed">
              Integrated package manager (<code className="text-amber-300">raven pkg search/add/publish</code>) backed by deterministic lockfiles (<code className="text-amber-300">raven.lock</code>).
            </p>
            <div className="pt-2 text-xs font-mono text-amber-400/90 flex items-center space-x-1">
              <span>Browse 20+ packages</span>
              <ArrowRight className="w-3 h-3" />
            </div>
          </div>

          {/* Pillar 6: Web Framework & Stdlib */}
          <div 
            onClick={() => onNavigate('stdlib')}
            className="dev-card rounded-xl p-6 space-y-3 cursor-pointer"
          >
            <div className="w-9 h-9 rounded-lg bg-rose-500/10 border border-rose-500/30 flex items-center justify-center text-rose-400">
              <Globe className="w-4 h-4" />
            </div>
            <h3 className="text-base font-bold text-white">Native Web Framework</h3>
            <p className="text-xs text-slate-400 leading-relaxed">
              Express-style HTTP routing, HTML Element Builder DSL, safe string templating, and 25+ standard library packages ready out-of-the-box.
            </p>
            <div className="pt-2 text-xs font-mono text-rose-400/90 flex items-center space-x-1">
              <span>View stdlib packages</span>
              <ArrowRight className="w-3 h-3" />
            </div>
          </div>
        </div>
      </section>

      {/* Code Showcase & Syntax Ergonomics */}
      <section className="px-6 max-w-7xl mx-auto">
        <div className="dev-panel rounded-2xl p-6 sm:p-8 space-y-6">
          <div className="flex flex-col md:flex-row md:items-center justify-between gap-4 border-b border-[#21262d] pb-6">
            <div>
              <span className="text-xs font-mono text-amber-400 font-semibold tracking-wider uppercase">
                Syntax In Action
              </span>
              <h2 className="text-2xl font-bold text-white mt-1">
                Idiomatic, Safe, and Expressive
              </h2>
            </div>
            <button
              onClick={() => onNavigate('playground')}
              className="px-4 py-2 rounded-md bg-[#161b22] hover:bg-[#21262d] text-xs font-mono text-amber-400 border border-[#30363d] transition-colors self-start cursor-pointer"
            >
              Open in Interactive Playground →
            </button>
          </div>

          <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
            {/* Snippet 1: Concurrency & Channels */}
            <div className="space-y-2">
              <div className="flex items-center justify-between text-xs font-mono text-slate-400 px-2">
                <span>01_concurrent_pipeline.rvn</span>
                <span className="text-emerald-400">CSP Concurrency</span>
              </div>
              <div className="terminal-window rounded-xl p-4 font-mono text-xs text-slate-200 leading-6 overflow-x-auto">
                <pre>{`import "stdlib/sync"

let ch = chan_create(10)
let wg = create_wait_group()

// Spawn background worker
wg.add(1)
spawn(fn() {
    let item = chan_recv(ch)
    print("Processed job:", item)
    wg.done()
})

chan_send(ch, {"task": "index_corpus", "id": 42})
wg.wait()
chan_close(ch)`}</pre>
              </div>
            </div>

            {/* Snippet 2: Web Server & Routing */}
            <div className="space-y-2">
              <div className="flex items-center justify-between text-xs font-mono text-slate-400 px-2">
                <span>02_web_server.rvn</span>
                <span className="text-sky-400">Native Web DSL</span>
              </div>
              <div className="terminal-window rounded-xl p-4 font-mono text-xs text-slate-200 leading-6 overflow-x-auto">
                <pre>{`import "stdlib/web/app"
import "stdlib/web/html"

let app = create_app()

app.get("/", fn(req) {
    let doc = html_doc("Raven App", [
        h1_el({}, "Raven Web Server"),
        p_el({}, "Running natively on Corvus VM.")
    ])
    return {"status": 200, "body": doc}
})

app.listen(8080)`}</pre>
              </div>
            </div>
          </div>
        </div>
      </section>

      {/* Benchmarks Preview Banner */}
      <section className="px-6 max-w-7xl mx-auto">
        <div className="dev-card rounded-2xl p-8 flex flex-col md:flex-row items-center justify-between gap-6">
          <div className="space-y-2 max-w-xl">
            <span className="text-xs font-mono text-emerald-400 font-bold uppercase tracking-wider">
              Empirical Performance
            </span>
            <h3 className="text-xl font-bold text-white">
              Sub-4ms Instant Startup. High Bytecode Throughput.
            </h3>
            <p className="text-xs text-slate-400 leading-relaxed">
              Corvus VM delivers 3.2x to 4.8x faster execution than tree-walking interpreters and consistently outpaces standard CPython 3.12 on compute-bound recursion and tight array iteration.
            </p>
          </div>
          <button
            onClick={() => onNavigate('benchmarks')}
            className="px-5 py-2.5 rounded-md bg-[#161b22] hover:bg-[#21262d] text-xs font-mono text-slate-200 hover:text-white border border-[#30363d] transition-colors shrink-0 cursor-pointer"
          >
            Explore Detailed Benchmarks →
          </button>
        </div>
      </section>
    </div>
  );
}
