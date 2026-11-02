import React, { useState } from 'react';
import { Copy, Check } from 'lucide-react';

interface CodeBlockProps {
  code: string;
  language?: string;
  filename?: string;
}

export default function CodeBlock({ code, language = 'raven', filename }: CodeBlockProps) {
  const [copied, setCopied] = useState(false);

  const handleCopy = () => {
    navigator.clipboard.writeText(code);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  return (
    <div className="rounded-xl overflow-hidden border border-slate-800 bg-[#090c14] my-4 shadow-xl">
      {filename && (
        <div className="px-4 py-2 bg-slate-900/90 border-b border-slate-800 flex items-center justify-between text-xs text-slate-400 font-mono">
          <span>{filename}</span>
          <button
            onClick={handleCopy}
            className="flex items-center space-x-1 hover:text-white transition-colors cursor-pointer"
          >
            {copied ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
            <span className={copied ? "text-emerald-400" : ""}>{copied ? "Copied" : "Copy"}</span>
          </button>
        </div>
      )}
      <div className="p-4 overflow-x-auto relative group">
        {!filename && (
          <button
            onClick={handleCopy}
            className="absolute top-3 right-3 p-1.5 rounded-lg bg-slate-800/80 hover:bg-slate-700 text-slate-400 hover:text-white transition-colors opacity-0 group-hover:opacity-100 cursor-pointer"
            title="Copy code"
          >
            {copied ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
          </button>
        )}
        <pre className="text-slate-200 text-xs md:text-[13px] font-mono leading-relaxed whitespace-pre">
          {code}
        </pre>
      </div>
    </div>
  );
}
