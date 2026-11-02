import React from 'react';

interface FooterProps {
  onNavigate: (page: string) => void;
}

export default function Footer({ onNavigate }: FooterProps) {
  const links = [
    { label: 'Home', page: 'home' },
    { label: 'Playground', page: 'playground' },
    { label: 'Packages', page: 'packages' },
    { label: 'Docs', page: 'docs' },
    { label: 'Standard Library', page: 'stdlib' },
    { label: 'Architecture', page: 'architecture' },
    { label: 'Benchmarks', page: 'benchmarks' },
  ];

  return (
    <footer className="mt-auto border-t border-[#21262d] py-10 px-6 bg-[#080a0f]">
      <div className="max-w-7xl mx-auto space-y-6">
        <div className="flex flex-col md:flex-row items-start md:items-center justify-between gap-6">
          {/* Brand */}
          <div className="flex items-center space-x-3">
            <img src="/logo.png" alt="Raven Logo" className="w-7 h-7 rounded-lg border border-slate-700/50" />
            <div>
              <span className="font-bold text-white text-sm">Raven Programming Language</span>
              <p className="text-xs text-slate-500 mt-0.5">v1.0.0 • 148 Tests Passing • MIT License</p>
            </div>
          </div>

          {/* Nav Links */}
          <div className="flex flex-wrap gap-x-5 gap-y-2">
            {links.map((l) => (
              <button
                key={l.page}
                onClick={() => onNavigate(l.page)}
                className="text-xs text-slate-400 hover:text-white transition-colors cursor-pointer"
              >
                {l.label}
              </button>
            ))}
            <a
              href="https://github.com/rs2big/Raven"
              target="_blank"
              rel="noreferrer"
              className="text-xs text-slate-400 hover:text-white transition-colors"
            >
              GitHub ↗
            </a>
          </div>
        </div>

        <div className="pt-4 border-t border-[#21262d]/70 flex flex-col md:flex-row items-center justify-between gap-3 text-[11px] text-slate-600 font-mono">
          <span>Copyright © 2026 Ran & The Raven Language Contributors. MIT License.</span>
          <span>Built with C++20 • Powered by the Corvus Engine</span>
        </div>
      </div>
    </footer>
  );
}
