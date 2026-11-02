import React from 'react';
import { ArrowRight } from 'lucide-react';

interface NavbarProps {
  currentPage: string;
  onNavigate: (page: string) => void;
}

export default function Navbar({ currentPage, onNavigate }: NavbarProps) {
  const navItems = [
    { id: 'home', label: 'Home' },
    { id: 'playground', label: 'Playground' },
    { id: 'docs', label: 'Documentation' },
    { id: 'stdlib', label: 'Standard Library' },
    { id: 'architecture', label: 'Architecture' },
    { id: 'benchmarks', label: 'Benchmarks' },
  ];

  return (
    <header className="sticky top-0 z-50 glass-panel border-b border-slate-800/80">
      <div className="max-w-7xl mx-auto px-6 h-18 flex items-center justify-between">
        {/* Brand */}
        <div 
          onClick={() => onNavigate('home')} 
          className="flex items-center space-x-3 cursor-pointer select-none group"
        >
          <img 
            src="/logo.png" 
            alt="Raven Logo" 
            className="w-9 h-9 rounded-lg shadow-md shadow-purple-900/40 group-hover:scale-105 transition-transform" 
          />
          <div className="flex items-baseline space-x-2">
            <span className="font-extrabold text-xl tracking-tight bg-gradient-to-r from-purple-400 via-indigo-200 to-cyan-400 bg-clip-text text-transparent">
              Raven
            </span>
            <span className="text-xs px-2 py-0.5 rounded-full bg-purple-500/20 text-purple-300 font-mono font-medium border border-purple-500/30">
              v1.1.0
            </span>
          </div>
        </div>

        {/* Navigation Links */}
        <nav className="hidden md:flex items-center space-x-1 lg:space-x-2 text-sm font-medium">
          {navItems.map((item) => {
            const isActive = currentPage === item.id;
            return (
              <button
                key={item.id}
                onClick={() => onNavigate(item.id)}
                className={`px-3 py-1.5 rounded-lg text-xs font-semibold transition-all cursor-pointer ${
                  isActive
                    ? 'bg-purple-600/30 text-purple-200 border border-purple-500/40 shadow-sm'
                    : 'text-slate-400 hover:text-white hover:bg-slate-800/60'
                }`}
              >
                {item.label}
              </button>
            );
          })}
        </nav>

        {/* Action Buttons */}
        <div className="flex items-center space-x-3">
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
          <button
            onClick={() => onNavigate('playground')}
            className="px-4 py-2 rounded-lg bg-gradient-to-r from-purple-600 to-indigo-600 hover:from-purple-500 hover:to-indigo-500 text-xs font-bold text-white shadow-lg shadow-purple-600/25 transition-all flex items-center space-x-1.5 cursor-pointer"
          >
            <span>Try Playground</span>
            <ArrowRight className="w-3.5 h-3.5" />
          </button>
        </div>
      </div>
    </header>
  );
}
