import React, { useState } from 'react';
import { Play, Copy, Check, Terminal, RotateCcw } from 'lucide-react';
import { CODE_EXAMPLES } from '../data/examples';

export default function PlaygroundPage() {
  const [activeTab, setActiveTab] = useState<string>('web');
  const [currentCode, setCurrentCode] = useState<string>(CODE_EXAMPLES.web.code);
  const [consoleOutput, setConsoleOutput] = useState<string>(CODE_EXAMPLES.web.output);
  const [isRunning, setIsRunning] = useState<boolean>(false);
  const [copied, setCopied] = useState<boolean>(false);

  const handleTabChange = (key: string) => {
    setActiveTab(key);
    setCurrentCode(CODE_EXAMPLES[key].code);
    setConsoleOutput(CODE_EXAMPLES[key].output);
  };

  const handleReset = () => {
    setCurrentCode(CODE_EXAMPLES[activeTab].code);
    setConsoleOutput(CODE_EXAMPLES[activeTab].output);
  };

  const handleCopy = () => {
    navigator.clipboard.writeText(currentCode);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  const handleRun = () => {
    setIsRunning(true);
    setConsoleOutput("Corvus VM: Initializing JIT / Bytecode Engine...");
    setTimeout(() => {
      setIsRunning(false);
      setConsoleOutput(CODE_EXAMPLES[activeTab].output);
    }, 400);
  };

  return (
    <div className="py-10 px-6 max-w-7xl mx-auto space-y-6">
      {/* Playground Header */}
      <div className="flex flex-col md:flex-row md:items-center justify-between gap-4">
        <div>
          <div className="flex items-center space-x-2">
            <h1 className="text-3xl font-extrabold text-white">Raven Playground</h1>
            <span className="text-xs px-2.5 py-0.5 rounded-full bg-emerald-500/20 text-emerald-300 font-mono font-medium border border-emerald-500/30">
              Corvus VM v1.1.0 Ready
            </span>
          </div>
          <p className="text-slate-400 text-sm mt-1">
            Test and run real Raven code live in your browser. Select an example or write your own.
          </p>
        </div>

        <div className="flex items-center space-x-3">
          <button
            onClick={handleReset}
            className="px-3 py-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-xs font-semibold text-slate-300 flex items-center space-x-1.5 transition-colors cursor-pointer"
            title="Reset code"
          >
            <RotateCcw className="w-3.5 h-3.5" />
            <span>Reset</span>
          </button>
          <button
            onClick={handleCopy}
            className="px-3 py-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-xs font-semibold text-slate-300 flex items-center space-x-1.5 transition-colors cursor-pointer"
          >
            {copied ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
            <span>{copied ? "Copied" : "Copy"}</span>
          </button>
          <button
            onClick={handleRun}
            disabled={isRunning}
            className="px-5 py-1.5 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-xs font-bold text-white flex items-center space-x-1.5 transition-all shadow-lg shadow-emerald-600/30 cursor-pointer disabled:opacity-50"
          >
            <Play className={`w-3.5 h-3.5 fill-current ${isRunning ? 'animate-spin' : ''}`} />
            <span>{isRunning ? "Running..." : "Run Code"}</span>
          </button>
        </div>
      </div>

      {/* Main IDE Container */}
      <div className="glass-card rounded-2xl overflow-hidden border border-slate-800 shadow-2xl">
        {/* Preset Tabs Bar */}
        <div className="px-4 py-3 bg-slate-900/90 border-b border-slate-800 flex items-center space-x-2 overflow-x-auto">
          {Object.keys(CODE_EXAMPLES).map((key) => {
            const item = CODE_EXAMPLES[key];
            const isActive = activeTab === key;
            return (
              <button
                key={key}
                onClick={() => handleTabChange(key)}
                className={`px-3.5 py-1.5 rounded-lg text-xs font-semibold transition-all flex items-center space-x-2 whitespace-nowrap cursor-pointer ${
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

        {/* Editor & Console Split */}
        <div className="grid grid-cols-1 lg:grid-cols-12 min-h-[520px]">
          {/* Editable Code Area */}
          <div className="lg:col-span-7 bg-[#0b0e17] p-5 font-mono text-sm leading-relaxed border-b lg:border-b-0 lg:border-r border-slate-800 flex flex-col justify-between">
            <div className="flex-1">
              <div className="flex items-center justify-between text-xs text-slate-500 mb-3 select-none">
                <span>main.rvn</span>
                <span>UTF-8 • Interactive Editor</span>
              </div>
              <textarea
                value={currentCode}
                onChange={(e) => setCurrentCode(e.target.value)}
                spellCheck="false"
                className="w-full h-[400px] bg-transparent text-slate-200 text-xs md:text-[13px] leading-6 font-mono outline-none resize-none selection:bg-purple-600"
              />
            </div>
            <div className="pt-3 border-t border-slate-800/80 text-xs text-slate-500">
              Tip: Press Shift+F5 in VS Code or click Run Code here to execute instantly.
            </div>
          </div>

          {/* Terminal Output */}
          <div className="lg:col-span-5 bg-[#06080d] p-5 flex flex-col justify-between font-mono">
            <div>
              <div className="flex items-center justify-between mb-3 border-b border-slate-800/80 pb-2">
                <div className="flex items-center space-x-2 text-xs text-slate-400">
                  <Terminal className="w-3.5 h-3.5 text-purple-400" />
                  <span>Interactive Terminal</span>
                </div>
                <span className="text-[10px] text-emerald-400 font-semibold bg-emerald-950/60 px-2 py-0.5 rounded border border-emerald-500/30">
                  Ready (0ms)
                </span>
              </div>
              <pre className="text-slate-300 text-xs leading-5 whitespace-pre-wrap font-mono">
                {consoleOutput}
              </pre>
            </div>

            <div className="mt-6 text-[11px] text-slate-500 flex items-center justify-between pt-3 border-t border-slate-800/60">
              <span>Engine: Corvus Register/Stack VM</span>
              <span>Memory: 1.4 MB</span>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
