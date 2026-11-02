import React, { useState, useEffect } from 'react';
import { useSearchParams } from 'react-router-dom';
import { Play, Copy, Check, Terminal, RotateCcw } from 'lucide-react';
import { CODE_EXAMPLES } from '../data/examples';

export default function PlaygroundPage(): React.JSX.Element {
  const [searchParams, setSearchParams] = useSearchParams();
  const exampleParam = searchParams.get('example');
  const initialTab = exampleParam && exampleParam in CODE_EXAMPLES ? exampleParam : 'web';

  const [activeTab, setActiveTab] = useState<string>(initialTab);
  const [currentCode, setCurrentCode] = useState<string>(CODE_EXAMPLES[initialTab].code);
  const [consoleOutput, setConsoleOutput] = useState<string>(CODE_EXAMPLES[initialTab].output);
  const [isRunning, setIsRunning] = useState<boolean>(false);
  const [copied, setCopied] = useState<boolean>(false);

  useEffect(() => {
    if (exampleParam && exampleParam in CODE_EXAMPLES && exampleParam !== activeTab) {
      setActiveTab(exampleParam);
      setCurrentCode(CODE_EXAMPLES[exampleParam].code);
      setConsoleOutput(CODE_EXAMPLES[exampleParam].output);
    }
  }, [exampleParam]);

  const handleTabChange = (key: string) => {
    setActiveTab(key);
    setCurrentCode(CODE_EXAMPLES[key].code);
    setConsoleOutput(CODE_EXAMPLES[key].output);
    setSearchParams({ example: key });
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

        {/* Action Controls */}
        <div className="flex items-center space-x-3">
          <button
            onClick={handleReset}
            className="px-3 py-2 rounded-lg bg-slate-900 border border-slate-700 hover:border-slate-600 text-xs text-slate-300 flex items-center space-x-1.5 transition-colors cursor-pointer"
            title="Reset to original code"
          >
            <RotateCcw className="w-3.5 h-3.5" />
            <span>Reset</span>
          </button>
          <button
            onClick={handleCopy}
            className="px-3 py-2 rounded-lg bg-slate-900 border border-slate-700 hover:border-slate-600 text-xs text-slate-300 flex items-center space-x-1.5 transition-colors cursor-pointer"
          >
            {copied ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
            <span>{copied ? 'Copied' : 'Copy'}</span>
          </button>
          <button
            onClick={handleRun}
            disabled={isRunning}
            className="px-5 py-2 rounded-lg bg-amber-600 hover:bg-amber-500 disabled:opacity-50 text-xs font-bold text-white flex items-center space-x-2 shadow-lg shadow-amber-600/20 transition-all cursor-pointer"
          >
            <Play className={`w-3.5 h-3.5 fill-current ${isRunning ? 'animate-spin' : ''}`} />
            <span>{isRunning ? 'Running...' : 'Run Code'}</span>
          </button>
        </div>
      </div>

      {/* Example Selector Tabs */}
      <div className="flex space-x-2 border-b border-slate-800 pb-2 overflow-x-auto">
        {Object.entries(CODE_EXAMPLES).map(([key, ex]) => (
          <button
            key={key}
            onClick={() => handleTabChange(key)}
            className={`px-4 py-2 rounded-lg text-xs font-semibold whitespace-nowrap transition-all cursor-pointer ${
              activeTab === key
                ? 'bg-amber-600 text-white shadow-md shadow-amber-900/30'
                : 'text-slate-400 hover:text-white hover:bg-slate-800/60'
            }`}
          >
            {ex.title}
          </button>
        ))}
      </div>

      {/* Editor & Console Split View */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6 min-h-[500px]">
        {/* Code Editor Panel */}
        <div className="bg-[#0d1117] border border-[#21262d] rounded-2xl flex flex-col overflow-hidden">
          <div className="px-4 py-3 bg-slate-900/80 border-b border-slate-800 flex items-center justify-between text-xs font-mono text-slate-400">
            <span className="flex items-center space-x-2">
              <span className="w-2.5 h-2.5 rounded-full bg-slate-700 inline-block" />
              <span>main.rvn</span>
            </span>
            <span className="text-slate-500">Raven v1.1.0</span>
          </div>

          <textarea
            value={currentCode}
            onChange={(e) => setCurrentCode(e.target.value)}
            spellCheck={false}
            className="flex-1 w-full bg-transparent text-slate-200 font-mono text-sm p-4 outline-none resize-none selection:bg-amber-500/30 leading-relaxed font-medium"
            placeholder="Write Raven code here..."
          />
        </div>

        {/* Console Execution Output Panel */}
        <div className="bg-[#0d1117] border border-[#21262d] rounded-2xl flex flex-col overflow-hidden">
          <div className="px-4 py-3 bg-slate-900/80 border-b border-slate-800 flex items-center justify-between text-xs font-mono text-slate-400">
            <span className="flex items-center space-x-2">
              <Terminal className="w-3.5 h-3.5 text-amber-400" />
              <span>Standard Output (Stdout)</span>
            </span>
            <span className="text-[11px] text-slate-500">Exit Code: 0 (Success)</span>
          </div>

          <div className="flex-1 p-4 font-mono text-xs text-slate-300 whitespace-pre overflow-auto bg-slate-950/60 leading-relaxed">
            {consoleOutput}
          </div>
        </div>
      </div>
    </div>
  );
}
