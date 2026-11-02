import React, { useState } from 'react';
import { BookOpen, ChevronRight, Hash } from 'lucide-react';
import { DOCS_SECTIONS, DocItem } from '../data/docs';
import CodeBlock from '../components/CodeBlock';

export default function DocsPage() {
  const [activeSectionId, setActiveSectionId] = useState<string>(DOCS_SECTIONS[0].items[0].id);

  let activeItem: DocItem | null = null;
  for (const sec of DOCS_SECTIONS) {
    for (const item of sec.items) {
      if (item.id === activeSectionId) {
        activeItem = item;
        break;
      }
    }
  }

  return (
    <div className="py-10 px-6 max-w-7xl mx-auto">
      <div className="grid grid-cols-1 md:grid-cols-12 gap-8">
        {/* Sidebar Navigation */}
        <aside className="md:col-span-4 lg:col-span-3 space-y-6">
          <div className="glass-panel p-4 rounded-xl border border-slate-800 sticky top-24">
            <div className="flex items-center space-x-2 text-sm font-bold text-white mb-4 pb-2 border-b border-slate-800">
              <BookOpen className="w-4 h-4 text-purple-400" />
              <span>Documentation</span>
            </div>

            <div className="space-y-4">
              {DOCS_SECTIONS.map((sec) => (
                <div key={sec.id} className="space-y-1">
                  <div className="text-[11px] font-bold text-slate-500 uppercase tracking-wider px-2">
                    {sec.title}
                  </div>
                  {sec.items.map((item) => {
                    const isActive = activeSectionId === item.id;
                    return (
                      <button
                        key={item.id}
                        onClick={() => setActiveSectionId(item.id)}
                        className={`w-full text-left px-2.5 py-1.5 rounded-lg text-xs font-medium transition-all flex items-center justify-between cursor-pointer ${
                          isActive
                            ? 'bg-purple-600/25 text-purple-200 border border-purple-500/40 font-semibold'
                            : 'text-slate-400 hover:text-white hover:bg-slate-800/50'
                        }`}
                      >
                        <span>{item.title}</span>
                        {isActive && <ChevronRight className="w-3.5 h-3.5 text-purple-400" />}
                      </button>
                    );
                  })}
                </div>
              ))}
            </div>
          </div>
        </aside>

        {/* Content Area */}
        <main className="md:col-span-8 lg:col-span-9">
          {activeItem && (
            <div className="glass-card p-8 rounded-2xl border border-slate-800 space-y-6">
              <div className="border-b border-slate-800 pb-4">
                <h1 className="text-3xl font-extrabold text-white flex items-center space-x-3">
                  <Hash className="w-6 h-6 text-purple-400" />
                  <span>{activeItem.title}</span>
                </h1>
              </div>

              {/* Render documentation content sections */}
              <div className="prose prose-invert max-w-none text-slate-300 text-sm leading-relaxed space-y-4">
                {activeItem.content.split('```').map((part, index) => {
                  if (index % 2 === 1) {
                    const lines = part.trim().split('\n');
                    const lang = lines[0];
                    const code = lines.slice(1).join('\n');
                    return <CodeBlock key={index} code={code} language={lang} />;
                  } else {
                    return (
                      <div key={index} className="space-y-3">
                        {part.split('\n\n').map((paragraph, pIdx) => {
                          if (paragraph.startsWith('### ')) {
                            return (
                              <h3 key={pIdx} className="text-lg font-bold text-white pt-2">
                                {paragraph.replace('### ', '')}
                              </h3>
                            );
                          }
                          return paragraph.trim() ? (
                            <p key={pIdx} className="text-slate-300 leading-relaxed">
                              {paragraph}
                            </p>
                          ) : null;
                        })}
                      </div>
                    );
                  }
                })}
              </div>
            </div>
          )}
        </main>
      </div>
    </div>
  );
}
