import React from 'react';
import { useSearchParams } from 'react-router-dom';
import { BookOpen, ChevronRight, Hash } from 'lucide-react';
import { DOCS_SECTIONS, DocItem } from '../data/docs';
import CodeBlock from '../components/CodeBlock';

export default function DocsPage(): React.JSX.Element {
  const [searchParams, setSearchParams] = useSearchParams();
  const activeSectionId = searchParams.get('section') || DOCS_SECTIONS[0].items[0].id;

  let activeItem: DocItem | null = null;
  for (const sec of DOCS_SECTIONS) {
    for (const item of sec.items) {
      if (item.id === activeSectionId) {
        activeItem = item;
        break;
      }
    }
  }

  // Fallback to first item if requested section not found
  if (!activeItem) {
    activeItem = DOCS_SECTIONS[0].items[0];
  }

  const handleSelectSection = (id: string) => {
    setSearchParams({ section: id });
  };

  return (
    <div className="py-10 px-6 max-w-7xl mx-auto">
      <div className="grid grid-cols-1 md:grid-cols-12 gap-8">
        {/* Sidebar Navigation */}
        <aside className="md:col-span-4 lg:col-span-3 space-y-6">
          <div className="bg-[#0d1117] border border-[#21262d] p-4 rounded-xl sticky top-24">
            <div className="flex items-center space-x-2 text-sm font-bold text-white mb-4 pb-2 border-b border-slate-800">
              <BookOpen className="w-4 h-4 text-amber-400" />
              <span>Documentation</span>
            </div>

            <div className="space-y-4">
              {DOCS_SECTIONS.map((sec) => (
                <div key={sec.id} className="space-y-1">
                  <div className="text-[11px] font-bold text-slate-500 uppercase tracking-wider px-2">
                    {sec.title}
                  </div>
                  {sec.items.map((item) => {
                    const isActive = activeItem?.id === item.id;
                    return (
                      <button
                        key={item.id}
                        onClick={() => handleSelectSection(item.id)}
                        className={`w-full text-left px-2.5 py-1.5 rounded-lg text-xs font-medium transition-all flex items-center justify-between cursor-pointer ${
                          isActive
                            ? 'bg-amber-500/10 text-amber-300 border border-amber-500/30 font-semibold'
                            : 'text-slate-400 hover:text-white hover:bg-slate-800/50'
                        }`}
                      >
                        <span>{item.title}</span>
                        {isActive && <ChevronRight className="w-3.5 h-3.5 text-amber-400" />}
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
            <div className="bg-[#0d1117] border border-[#21262d] p-8 rounded-2xl space-y-6">
              <div className="border-b border-slate-800 pb-4">
                <h1 className="text-3xl font-extrabold text-white flex items-center space-x-3">
                  <Hash className="w-6 h-6 text-amber-400" />
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
