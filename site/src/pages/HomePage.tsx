import React, { useState } from 'react';
import {
  Sparkles, Copy, Check, ArrowRight, Cpu, Layers, Globe,
  ShieldCheck, Bug, Box
} from 'lucide-react';

interface HomePageProps {
  onNavigate: (page: string) => void;
}

export default function HomePage({ onNavigate }: HomePageProps) {
  const [copiedInstall, setCopiedInstall] = useState(false);
  const installCmd = "curl -fsSL https://raven-lang.org/install.sh | bash";

  const handleCopyInstall = () => {
    navigator.clipboard.writeText(installCmd);
    setCopiedInstall(true);
    setTimeout(() => setCopiedInstall(false), 2000);
  };

  return (
    <div className="space-y-24 pb-20">
      {/* Hero Section */}
      <section className="relative z-10 pt-20 px-6 max-w-7xl mx-auto text-center">
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
        <div className="mt-10 max-w-xl mx-auto">
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

        {/* Action Buttons */}
        <div className="mt-8 flex flex-wrap items-center justify-center gap-4">
          <button
            onClick={() => onNavigate('playground')}
            className="px-6 py-3 rounded-xl bg-gradient-to-r from-purple-600 to-indigo-600 hover:from-purple-500 hover:to-indigo-500 text-sm font-bold text-white shadow-xl shadow-purple-600/30 transition-all flex items-center space-x-2 cursor-pointer"
          >
            <span>Open Live Playground</span>
            <ArrowRight className="w-4 h-4" />
          </button>
          <button
            onClick={() => onNavigate('docs')}
            className="px-6 py-3 rounded-xl bg-slate-800/80 hover:bg-slate-800 text-sm font-semibold text-slate-200 border border-slate-700 transition-colors cursor-pointer"
          >
            Explore Documentation
          </button>
        </div>
      </section>

      {/* Feature Pillars */}
      <section className="px-6 max-w-7xl mx-auto">
        <div className="text-center mb-16">
          <h2 className="text-3xl md:text-4xl font-extrabold tracking-tight">
            Engineered for Modern Software
          </h2>
          <p className="mt-3 text-slate-400 max-w-xl mx-auto text-sm">
            Everything you need for microservices, web apps, tools, and background workloads without installing 50 dependencies.
          </p>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
          <div 
            onClick={() => onNavigate('architecture')}
            className="glass-panel p-6 rounded-xl hover:border-purple-500/50 transition-all group cursor-pointer"
          >
            <div className="w-10 h-10 rounded-lg bg-purple-500/10 border border-purple-500/30 flex items-center justify-center text-purple-400 mb-4 group-hover:scale-105 transition-transform">
              <Cpu className="w-5 h-5" />
            </div>
            <h3 className="text-lg font-bold text-white mb-2">Dual Execution Engine</h3>
            <p className="text-sm text-slate-400 leading-relaxed">
              Instant Tree-Walk Interpreter for REPL and prototyping, paired with the Corvus Bytecode VM and bytecode serializer for maximum throughput.
            </p>
          </div>

          <div 
            onClick={() => onNavigate('docs')}
            className="glass-panel p-6 rounded-xl hover:border-cyan-500/50 transition-all group cursor-pointer"
          >
            <div className="w-10 h-10 rounded-lg bg-cyan-500/10 border border-cyan-500/30 flex items-center justify-center text-cyan-400 mb-4 group-hover:scale-105 transition-transform">
              <Layers className="w-5 h-5" />
            </div>
            <h3 className="text-lg font-bold text-white mb-2">Real CSP Multi-threading</h3>
            <p className="text-sm text-slate-400 leading-relaxed">
              Native OS threads, typed channels with buffering, WaitGroups, and worker pools. True parallelism without the limitations of a GIL.
            </p>
          </div>

          <div 
            onClick={() => onNavigate('stdlib')}
            className="glass-panel p-6 rounded-xl hover:border-emerald-500/50 transition-all group cursor-pointer"
          >
            <div className="w-10 h-10 rounded-lg bg-emerald-500/10 border border-emerald-500/30 flex items-center justify-center text-emerald-400 mb-4 group-hover:scale-105 transition-transform">
              <Globe className="w-5 h-5" />
            </div>
            <h3 className="text-lg font-bold text-white mb-2">Native Web Framework</h3>
            <p className="text-sm text-slate-400 leading-relaxed">
              Built-in <code className="text-emerald-300 font-mono text-xs">stdlib/web</code> with Express-style routing, HTML Element Builder DSL, and string templating out of the box.
            </p>
          </div>

          <div 
            onClick={() => onNavigate('docs')}
            className="glass-panel p-6 rounded-xl hover:border-amber-500/50 transition-all group cursor-pointer"
          >
            <div className="w-10 h-10 rounded-lg bg-amber-500/10 border border-amber-500/30 flex items-center justify-center text-amber-400 mb-4 group-hover:scale-105 transition-transform">
              <ShieldCheck className="w-5 h-5" />
            </div>
            <h3 className="text-lg font-bold text-white mb-2">Gradual Type Annotations</h3>
            <p className="text-sm text-slate-400 leading-relaxed">
              Type-safe when building critical systems, dynamic when prototyping. Verified at declaration, call boundaries, and execution.
            </p>
          </div>

          <div 
            onClick={() => onNavigate('docs')}
            className="glass-panel p-6 rounded-xl hover:border-indigo-500/50 transition-all group cursor-pointer"
          >
            <div className="w-10 h-10 rounded-lg bg-indigo-500/10 border border-indigo-500/30 flex items-center justify-center text-indigo-400 mb-4 group-hover:scale-105 transition-transform">
              <Bug className="w-5 h-5" />
            </div>
            <h3 className="text-lg font-bold text-white mb-2">rvndb & VS Code Debugger</h3>
            <p className="text-sm text-slate-400 leading-relaxed">
              Full F5 debugging, gutter breakpoints, call stack inspection, and expression evaluation right inside your editor.
            </p>
          </div>

          <div 
            onClick={() => onNavigate('stdlib')}
            className="glass-panel p-6 rounded-xl hover:border-pink-500/50 transition-all group cursor-pointer"
          >
            <div className="w-10 h-10 rounded-lg bg-pink-500/10 border border-pink-500/30 flex items-center justify-center text-pink-400 mb-4 group-hover:scale-105 transition-transform">
              <Box className="w-5 h-5" />
            </div>
            <h3 className="text-lg font-bold text-white mb-2">Production Standard Library</h3>
            <p className="text-sm text-slate-400 leading-relaxed">
              Min-Heap PriorityQueue, Trie, BST, Deque, HMAC-SHA256 crypto, TCP/UDP sockets, JSON, CSV, Regex, and package manager.
            </p>
          </div>
        </div>
      </section>

      {/* Code Snippet Spotlight */}
      <section className="px-6 max-w-7xl mx-auto">
        <div className="glass-card rounded-2xl p-8 border border-slate-800">
          <div className="flex flex-col lg:flex-row items-center justify-between gap-8">
            <div className="max-w-xl space-y-4">
              <span className="text-xs font-mono text-cyan-400 uppercase tracking-widest font-semibold">
                Ergonomics In Action
              </span>
              <h2 className="text-3xl font-extrabold text-white">
                Clean, expressively modern code you actually enjoy writing.
              </h2>
              <p className="text-slate-400 text-sm leading-relaxed">
                No semicolon tax, no mandatory condition parentheses, native slicing, and first-class destructuring. Everything is tuned for fast, readable programming.
              </p>
              <div className="pt-2">
                <button
                  onClick={() => onNavigate('playground')}
                  className="px-5 py-2.5 rounded-lg bg-purple-600 hover:bg-purple-500 text-xs font-bold text-white flex items-center space-x-2 transition-colors cursor-pointer"
                >
                  <span>Open In Playground</span>
                  <ArrowRight className="w-3.5 h-3.5" />
                </button>
              </div>
            </div>

            <div className="w-full lg:w-1/2 rounded-xl overflow-hidden border border-slate-800 bg-[#090c14] p-5 font-mono text-xs leading-6 text-slate-200">
              <pre className="overflow-x-auto whitespace-pre">
{`// Seamless Modern Raven Syntax
let users = ["Alice", "Bob", "Charlie", "Diana"]
let [admin, lead] = users[:2] // Slice + Destructure

fn greet(user, role: String = "Engineer") {
    print("Welcome " + user + " (" + role + ")!")
}

for user in users {
    greet(user)
}`}
              </pre>
            </div>
          </div>
        </div>
      </section>
    </div>
  );
}
