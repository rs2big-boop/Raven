import React from 'react';

interface FooterProps {
  onNavigate: (page: string) => void;
}

export default function Footer({ onNavigate }: FooterProps) {
  return (
    <footer className="mt-auto border-t border-slate-800/80 py-12 px-6 bg-[#05070a]">
      <div className="max-w-7xl mx-auto flex flex-col md:flex-row items-center justify-between gap-6 text-xs text-slate-500">
        <div className="flex items-center space-x-3">
          <img src="/logo.png" alt="Raven Logo" className="w-6 h-6 rounded" />
          <span className="font-semibold text-slate-300">The Raven Programming Language</span>
          <span>• MIT License</span>
        </div>
        <div className="flex flex-wrap items-center justify-center gap-6">
          <button onClick={() => onNavigate('home')} className="hover:text-slate-300 transition-colors cursor-pointer">Home</button>
          <button onClick={() => onNavigate('playground')} className="hover:text-slate-300 transition-colors cursor-pointer">Playground</button>
          <button onClick={() => onNavigate('docs')} className="hover:text-slate-300 transition-colors cursor-pointer">Documentation</button>
          <button onClick={() => onNavigate('stdlib')} className="hover:text-slate-300 transition-colors cursor-pointer">Standard Library</button>
          <button onClick={() => onNavigate('architecture')} className="hover:text-slate-300 transition-colors cursor-pointer">Architecture</button>
          <button onClick={() => onNavigate('benchmarks')} className="hover:text-slate-300 transition-colors cursor-pointer">Benchmarks</button>
          <a href="https://github.com" target="_blank" rel="noreferrer" className="hover:text-slate-300 transition-colors">GitHub</a>
        </div>
      </div>
    </footer>
  );
}
