import React, { useState, useMemo } from 'react';
import { Search, Package, Download, Star, ExternalLink, Copy, Check, Terminal, ShieldCheck, Tag, UploadCloud } from 'lucide-react';
import { COMMUNITY_PACKAGES, RavenPackage } from '../data/packages';

export default function PackagesPage(): React.JSX.Element {
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedCategory, setSelectedCategory] = useState<string>('all');
  const [copiedId, setCopiedId] = useState<string | null>(null);

  const categories = [
    { id: 'all', label: 'All Packages' },
    { id: 'web', label: 'Web & APIs' },
    { id: 'concurrency', label: 'Concurrency' },
    { id: 'database', label: 'Databases' },
    { id: 'security', label: 'Security & Auth' },
    { id: 'cli', label: 'CLI & Terminal' },
    { id: 'data', label: 'Data & Serialization' },
    { id: 'tools', label: 'Developer Tools' }
  ];

  const filteredPackages = useMemo(() => {
    return COMMUNITY_PACKAGES.filter((pkg) => {
      const matchesCategory = selectedCategory === 'all' || pkg.category === selectedCategory;
      const q = searchQuery.toLowerCase();
      const matchesSearch =
        pkg.name.toLowerCase().includes(q) ||
        pkg.description.toLowerCase().includes(q) ||
        pkg.tags.some((t) => t.toLowerCase().includes(q)) ||
        pkg.author.toLowerCase().includes(q);
      return matchesCategory && matchesSearch;
    });
  }, [searchQuery, selectedCategory]);

  const handleCopy = (id: string, text: string) => {
    navigator.clipboard.writeText(text);
    setCopiedId(id);
    setTimeout(() => setCopiedId(null), 2000);
  };

  return (
    <div className="py-12 px-6 max-w-7xl mx-auto space-y-12">
      {/* Hero Header */}
      <div className="space-y-4 max-w-3xl">
        <div className="inline-flex items-center space-x-2 px-3 py-1 rounded-md bg-[#161d28] border border-amber-500/30 text-amber-400 font-mono text-xs">
          <Package className="w-3.5 h-3.5 text-amber-400" />
          <span>Official Package Registry & Ecosystem Hub</span>
        </div>
        <h1 className="text-4xl font-extrabold tracking-tight text-white sm:text-5xl">
          Ecosystem & Packages
        </h1>
        <p className="text-slate-400 text-base leading-relaxed">
          Discover, install, and share reusable libraries built for the Raven programming language.
          Managed via deterministic lockfiles (<code className="text-amber-300 font-mono text-xs">raven.lock</code>) and instant CLI commands.
        </p>
      </div>

      {/* CLI Quick Reference Terminal */}
      <div className="terminal-window rounded-xl p-5 font-mono text-xs text-slate-300 space-y-2">
        <div className="flex items-center space-x-2 text-slate-500 pb-2 border-b border-slate-800 text-[11px]">
          <Terminal className="w-3.5 h-3.5 text-amber-400" />
          <span>Terminal • Raven Package Manager CLI</span>
        </div>
        <div className="grid grid-cols-1 md:grid-cols-3 gap-4 pt-1">
          <div>
            <span className="text-slate-500"># Search packages:</span>
            <div className="text-amber-400 mt-0.5">$ raven pkg search router</div>
          </div>
          <div>
            <span className="text-slate-500"># Add dependency:</span>
            <div className="text-emerald-400 mt-0.5">$ raven pkg add raven-router</div>
          </div>
          <div>
            <span className="text-slate-500"># Publish your package:</span>
            <div className="text-cyan-400 mt-0.5">$ raven pkg publish</div>
          </div>
        </div>
      </div>

      {/* Search & Category Filter */}
      <div className="space-y-4">
        <div className="relative">
          <Search className="absolute left-4 top-1/2 -translate-y-1/2 w-4 h-4 text-slate-500" />
          <input
            type="text"
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            placeholder="Search packages by name, description, author, or tag (e.g. 'router', 'jwt', 'postgres')..."
            className="w-full pl-11 pr-4 py-3 bg-[#0d1117] border border-[#21262d] rounded-xl text-sm text-slate-100 placeholder-slate-500 focus:outline-none focus:border-amber-500/80 transition-colors"
          />
        </div>

        {/* Category Pills */}
        <div className="flex flex-wrap gap-2 pt-1">
          {categories.map((cat) => (
            <button
              key={cat.id}
              onClick={() => setSelectedCategory(cat.id)}
              className={`px-3 py-1.5 rounded-lg text-xs font-medium transition-colors cursor-pointer ${
                selectedCategory === cat.id
                  ? 'bg-amber-500/15 text-amber-300 border border-amber-500/40'
                  : 'bg-[#0d1117] text-slate-400 border border-[#21262d] hover:text-white hover:border-slate-700'
              }`}
            >
              {cat.label}
            </button>
          ))}
        </div>
      </div>

      {/* Package Grid */}
      <div className="space-y-4">
        <div className="flex items-center justify-between text-xs text-slate-400 font-mono">
          <span>Showing {filteredPackages.length} packages</span>
          <span>Verified Registry • SemVer Compliant</span>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-5">
          {filteredPackages.map((pkg) => (
            <div
              key={pkg.id}
              className="dev-card rounded-xl p-5 flex flex-col justify-between space-y-4"
            >
              <div className="space-y-2.5">
                <div className="flex items-start justify-between">
                  <div>
                    <h3 className="font-bold text-base text-white hover:text-amber-400 transition-colors cursor-pointer flex items-center space-x-1.5">
                      <span>{pkg.name}</span>
                      <ShieldCheck className="w-3.5 h-3.5 text-amber-400" />
                    </h3>
                    <div className="text-xs text-slate-500 font-mono mt-0.5">
                      v{pkg.version} • by @{pkg.author}
                    </div>
                  </div>
                  {pkg.featured && (
                    <span className="px-2 py-0.5 text-[10px] font-mono uppercase tracking-wider font-bold rounded bg-amber-500/10 text-amber-300 border border-amber-500/30">
                      Featured
                    </span>
                  )}
                </div>

                <p className="text-xs text-slate-400 leading-relaxed">
                  {pkg.description}
                </p>

                {/* Tags */}
                <div className="flex flex-wrap gap-1.5 pt-1">
                  {pkg.tags.map((t) => (
                    <span
                      key={t}
                      className="px-2 py-0.5 rounded bg-[#161b22] text-[#8b949e] text-[10px] font-mono border border-[#30363d]"
                    >
                      #{t}
                    </span>
                  ))}
                </div>
              </div>

              {/* Footer Metrics & Install Action */}
              <div className="space-y-3 pt-3 border-t border-[#21262d]/80">
                <div className="flex items-center justify-between text-xs text-slate-400 font-mono">
                  <div className="flex items-center space-x-3">
                    <span className="flex items-center space-x-1">
                      <Star className="w-3.5 h-3.5 text-amber-400/80" />
                      <span>{pkg.stars}</span>
                    </span>
                    <span className="flex items-center space-x-1">
                      <Download className="w-3.5 h-3.5 text-emerald-400/80" />
                      <span>{(pkg.downloads / 1000).toFixed(1)}k</span>
                    </span>
                  </div>
                  <a
                    href={`https://${pkg.repository}`}
                    target="_blank"
                    rel="noreferrer"
                    className="text-slate-400 hover:text-white flex items-center space-x-1 transition-colors"
                  >
                    <span>repo</span>
                    <ExternalLink className="w-3 h-3" />
                  </a>
                </div>

                {/* Copy Command Button */}
                <div
                  onClick={() => handleCopy(pkg.id, pkg.installCommand)}
                  className="flex items-center justify-between px-3 py-2 rounded-lg bg-[#06090e] border border-[#21262d] text-xs font-mono text-slate-300 hover:border-amber-500/50 transition-colors cursor-pointer group"
                >
                  <span className="text-slate-400 truncate">{pkg.installCommand}</span>
                  <button className="text-slate-400 group-hover:text-amber-400 pl-2">
                    {copiedId === pkg.id ? (
                      <Check className="w-3.5 h-3.5 text-emerald-400" />
                    ) : (
                      <Copy className="w-3.5 h-3.5" />
                    )}
                  </button>
                </div>
              </div>
            </div>
          ))}
        </div>
      </div>

      {/* Publish Guide Section */}
      <div className="dev-panel rounded-2xl p-8 space-y-6">
        <div className="flex items-center space-x-3">
          <div className="w-10 h-10 rounded-xl bg-amber-500/10 border border-amber-500/30 flex items-center justify-center text-amber-400">
            <UploadCloud className="w-5 h-5" />
          </div>
          <div>
            <h2 className="text-xl font-bold text-white">How to Publish Your Package</h2>
            <p className="text-xs text-slate-400">Join the Raven community and share your libraries with developers worldwide.</p>
          </div>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-4 gap-4 text-xs font-mono">
          <div className="p-4 rounded-xl bg-[#080b10] border border-[#21262d] space-y-2">
            <div className="text-amber-400 font-bold">1. Initialize Package</div>
            <p className="text-slate-400 text-[11px] leading-relaxed">
              Create a standardized manifest with:
            </p>
            <div className="text-slate-200 bg-[#0d1117] p-2 rounded border border-[#21262d]">
              $ raven pkg init my-lib
            </div>
          </div>

          <div className="p-4 rounded-xl bg-[#080b10] border border-[#21262d] space-y-2">
            <div className="text-amber-400 font-bold">2. Implement Library</div>
            <p className="text-slate-400 text-[11px] leading-relaxed">
              Write idiomatic Raven code in <code className="text-amber-300">src/main.rvn</code> or <code className="text-amber-300">lib/</code>.
            </p>
            <div className="text-slate-200 bg-[#0d1117] p-2 rounded border border-[#21262d]">
              fn my_func() &#123; ... &#125;
            </div>
          </div>

          <div className="p-4 rounded-xl bg-[#080b10] border border-[#21262d] space-y-2">
            <div className="text-amber-400 font-bold">3. Test Locally</div>
            <p className="text-slate-400 text-[11px] leading-relaxed">
              Run local tests to ensure zero regressions before release:
            </p>
            <div className="text-slate-200 bg-[#0d1117] p-2 rounded border border-[#21262d]">
              $ raven test
            </div>
          </div>

          <div className="p-4 rounded-xl bg-[#080b10] border border-[#21262d] space-y-2">
            <div className="text-amber-400 font-bold">4. Publish</div>
            <p className="text-slate-400 text-[11px] leading-relaxed">
              Validate and publish to the community registry:
            </p>
            <div className="text-slate-200 bg-[#0d1117] p-2 rounded border border-[#21262d]">
              $ raven pkg publish
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
