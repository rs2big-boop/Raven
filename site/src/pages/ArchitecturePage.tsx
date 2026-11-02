import React from 'react';
import { Terminal } from 'lucide-react';

export default function ArchitecturePage() {
  const subsystems = [
    {
      title: "1. Language Frontend",
      desc: "Lexer, Tokenizer, Recursive Descent Parser & Diagnostic Engine with ANSI error carets.",
      components: ["lexer.cpp / token.cpp", "parser.cpp / parser_error.cpp", "ast/expressions.cpp", "ast/statements.cpp", "diagnostics/diagnostic.cpp"]
    },
    {
      title: "2. Semantic & IR Optimization",
      desc: "Scope resolution, symbol tables, gradual type validation, and SSA-like 3-address IR with optimization passes.",
      components: ["semantic/resolver.cpp", "semantic/types.cpp", "ir/ir_builder.cpp", "passes/constant_folding.cpp", "passes/dead_code_elimination.cpp", "passes/common_subexpression.cpp"]
    },
    {
      title: "3. Dual Execution Backends",
      desc: "Instant Tree-Walk Interpreter with environment frames and Corvus Register/Stack Bytecode Virtual Machine with binary serializer.",
      components: ["interpreter/evaluator.cpp", "interpreter/environment.cpp", "compiler/compiler.cpp", "vm/engine.cpp / stack.cpp", "compiler/bytecode_serializer.cpp"]
    },
    {
      title: "4. Concurrency & Runtime",
      desc: "True OS multi-threading, CSP channels, WaitGroups, mutexes, non-blocking sockets, and managed heap with cycle collector.",
      components: ["concurrency/channel.cpp", "concurrency/thread.cpp", "concurrency/wait_group.cpp", "memory/managed_heap.cpp", "memory/cycle_collector.cpp", "platform/sockets.cpp"]
    }
  ];

  return (
    <div className="py-10 px-6 max-w-7xl mx-auto space-y-12">
      {/* Header */}
      <div className="text-center max-w-3xl mx-auto">
        <h1 className="text-3xl md:text-5xl font-extrabold text-white">
          Raven Engine Architecture
        </h1>
        <p className="mt-3 text-slate-400 text-sm md:text-base leading-relaxed">
          High-performance C++20 design structured with strict modularity between the compiler frontend, IR optimizer, dual execution backends, and CSP runtime.
        </p>
      </div>

      {/* Subsystem Pipeline Cards */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
        {subsystems.map((sub, idx) => (
          <div key={idx} className="bg-[#0d1117] border border-[#21262d] p-6 rounded-2xl space-y-4">
            <div className="flex items-center space-x-3">
              <div className="w-8 h-8 rounded-lg bg-amber-500/10 border border-amber-500/30 flex items-center justify-center font-bold text-xs text-amber-400">
                {idx + 1}
              </div>
              <h3 className="text-lg font-bold text-white">{sub.title}</h3>
            </div>
            <p className="text-xs text-slate-400 leading-relaxed">{sub.desc}</p>
            
            <div className="pt-2">
              <div className="text-[11px] font-bold text-slate-500 uppercase tracking-wider mb-2">Key Modules</div>
              <div className="flex flex-wrap gap-1.5 font-mono text-[10px]">
                {sub.components.map((c) => (
                  <span key={c} className="px-2 py-0.5 rounded bg-slate-900 text-slate-300 border border-slate-800">
                    {c}
                  </span>
                ))}
              </div>
            </div>
          </div>
        ))}
      </div>

      {/* Execution Flow Diagram */}
      <div className="bg-[#0d1117] border border-[#21262d] p-8 rounded-2xl space-y-6">
        <h2 className="text-xl font-bold text-white flex items-center space-x-2">
          <Terminal className="w-5 h-5 text-cyan-400" />
          <span>Pipeline Data Flow</span>
        </h2>
        
        <div className="p-4 rounded-xl bg-[#090c14] border border-slate-800 font-mono text-xs text-slate-300 leading-6 overflow-x-auto">
          <pre>{`[ Source Code: .rvn ]
       │
       ▼
[ Lexer & Tokenizer ] ──> [ Diagnostic Engine (ANSI Carets) ]
       │
       ▼
[ Recursive Descent Parser ] ──> [ AST Hierarchy ]
       │
       ├───> [ Semantic Analyzer & Gradual Type Checker ]
       │            │
       │            ▼
       │     [ IR 3-Address Code Builder ]
       │            │
       │            ▼
       │     [ Optimization Passes: Constant Fold, DCE, CSE ]
       │
       ├───> [ Tree-Walk Interpreter ] ──> (Zero-compile REPL & Scripts)
       │            │
       │            ▼
       └───> [ Bytecode Compiler ] ──> [ Bytecode Serializer: .rvc ]
                    │
                    ▼
             [ Corvus Register/Stack VM ]
                    │
                    ▼
     [ Native OS Runtimes & CSP Concurrency ]`}</pre>
        </div>
      </div>
    </div>
  );
}
