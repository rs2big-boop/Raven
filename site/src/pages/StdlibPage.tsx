import React, { useState } from 'react';
import { Box, Search } from 'lucide-react';
import { STDLIB_PACKAGES } from '../data/stdlib';
import CodeBlock from '../components/CodeBlock';

export default function StdlibPage() {
  const [searchTerm, setSearchTerm] = useState<string>('');
  const [selectedCategory, setSelectedCategory] = useState<string>('All');

  const categories = ['All', 'Web & Networking', 'Concurrency', 'Data Structures', 'Core Utilities', 'Security', 'System'];

  const filteredPackages = STDLIB_PACKAGES.filter((pkg) => {
    const matchesSearch = pkg.name.toLowerCase().includes(searchTerm.toLowerCase()) ||
                          pkg.description.toLowerCase().includes(searchTerm.toLowerCase()) ||
                          pkg.functions.some(f => f.toLowerCase().includes(searchTerm.toLowerCase()));
    const matchesCategory = selectedCategory === 'All' || pkg.category === selectedCategory;
    return matchesSearch && matchesCategory;
  });

  return (
    <div className="py-10 px-6 max-w-7xl mx-auto space-y-8">
      {/* Header */}
      <div>
        <h1 className="text-3xl md:text-4xl font-extrabold text-white flex items-center space-x-3">
          <Box className="w-8 h-8 text-amber-400" />
          <span>Standard Library Index</span>
        </h1>
        <p className="mt-2 text-slate-400 text-sm max-w-2xl">
          Raven includes a battery-included, Go-inspired standard library covering concurrency, web, networking, crypto, and rich data structures with zero external dependencies.
        </p>
      </div>

      {/* Filter and Search Bar */}
      <div className="bg-[#0d1117] border border-[#21262d] p-4 rounded-xl flex flex-col md:flex-row items-center justify-between gap-4">
        <div className="relative w-full md:w-80">
          <Search className="w-4 h-4 text-slate-500 absolute left-3 top-3" />
          <input
            type="text"
            placeholder="Search packages, functions..."
            value={searchTerm}
            onChange={(e) => setSearchTerm(e.target.value)}
            className="w-full pl-9 pr-4 py-2 bg-slate-900 border border-slate-700 rounded-lg text-xs text-white placeholder-slate-500 outline-none focus:border-amber-500 transition-colors"
          />
        </div>

        {/* Category Pills */}
        <div className="flex items-center space-x-2 overflow-x-auto w-full md:w-auto pb-2 md:pb-0">
          {categories.map((cat) => (
            <button
              key={cat}
              onClick={() => setSelectedCategory(cat)}
              className={`px-3 py-1.5 rounded-lg text-xs font-semibold whitespace-nowrap transition-colors cursor-pointer ${
                selectedCategory === cat
                  ? 'bg-amber-600 text-white'
                  : 'text-slate-400 hover:text-white hover:bg-slate-800'
              }`}
            >
              {cat}
            </button>
          ))}
        </div>
      </div>

      {/* Packages Grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
        {filteredPackages.map((pkg) => (
          <div key={pkg.name} className="bg-[#0d1117] border border-[#21262d] p-6 rounded-2xl space-y-4 hover:border-slate-700 transition-all flex flex-col justify-between">
            <div className="space-y-3">
              <div className="flex items-center justify-between">
                <span className="font-mono font-bold text-base text-amber-400">{pkg.name}</span>
                <span className="text-[11px] px-2 py-0.5 rounded bg-slate-800 text-slate-400 border border-slate-700">
                  {pkg.category}
                </span>
              </div>
              <p className="text-xs text-slate-400 leading-relaxed">{pkg.description}</p>
              
              {/* Function list */}
              <div className="flex flex-wrap gap-1.5 pt-1">
                {pkg.functions.map((fnName) => (
                  <span key={fnName} className="font-mono text-[10px] px-2 py-0.5 rounded bg-slate-900/90 text-cyan-300 border border-slate-800">
                    {fnName}
                  </span>
                ))}
              </div>
            </div>

            {/* Quick snippet */}
            <div className="pt-3 border-t border-slate-800/80">
              <CodeBlock code={pkg.snippet} />
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}
