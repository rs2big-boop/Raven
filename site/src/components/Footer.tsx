import React from 'react';
import { Link } from 'react-router-dom';

export default function Footer(): React.JSX.Element {
  const links = [
    { label: 'Home', path: '/' },
    { label: 'Playground', path: '/playground' },
    { label: 'Packages', path: '/packages' },
    { label: 'Docs', path: '/docs' },
    { label: 'Standard Library', path: '/stdlib' },
    { label: 'Architecture', path: '/architecture' },
    { label: 'Benchmarks', path: '/benchmarks' },
  ];

  return (
    <footer className="mt-auto border-t border-[#21262d] py-10 px-6 bg-[#080a0f]">
      <div className="max-w-7xl mx-auto space-y-6">
        <div className="flex flex-col md:flex-row items-start md:items-center justify-between gap-6">
          {/* Brand */}
          <Link to="/" className="flex items-center space-x-3 group">
            <img src="/logo.png" alt="Raven Logo" className="w-7 h-7 rounded-lg border border-slate-700/50 group-hover:scale-105 transition-transform" />
            <div>
              <span className="font-bold text-white text-sm group-hover:text-amber-400 transition-colors">Raven Programming Language</span>
              <p className="text-xs text-slate-500 mt-0.5">v1.0.0 • 150 Tests Passing • MIT License</p>
            </div>
          </Link>

          {/* Nav Links */}
          <div className="flex flex-wrap gap-x-5 gap-y-2">
            {links.map((l) => (
              <Link
                key={l.path}
                to={l.path}
                className="text-xs text-slate-400 hover:text-white transition-colors"
              >
                {l.label}
              </Link>
            ))}
            <a
              href="https://github.com/rs2big-boop/Raven"
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
