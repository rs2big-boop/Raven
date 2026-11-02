import React, { useState } from 'react';
import Navbar from './components/Navbar';
import Footer from './components/Footer';
import HomePage from './pages/HomePage';
import PlaygroundPage from './pages/PlaygroundPage';
import DocsPage from './pages/DocsPage';
import StdlibPage from './pages/StdlibPage';
import ArchitecturePage from './pages/ArchitecturePage';
import BenchmarksPage from './pages/BenchmarksPage';

export default function App(): React.JSX.Element {
  const [currentPage, setCurrentPage] = useState<string>('home');

  // Scroll to top upon page navigation
  const handleNavigate = (pageId: string) => {
    setCurrentPage(pageId);
    window.scrollTo({ top: 0, behavior: 'smooth' });
  };

  return (
    <div className="min-h-screen bg-[#07090e] text-slate-100 flex flex-col selection:bg-purple-600 selection:text-white">
      {/* Background ambient lighting */}
      <div className="fixed inset-0 pointer-events-none overflow-hidden z-0">
        <div className="absolute top-[-10%] left-[20%] w-[600px] h-[600px] bg-purple-600/10 rounded-full blur-[140px]" />
        <div className="absolute top-[40%] right-[10%] w-[500px] h-[500px] bg-cyan-500/10 rounded-full blur-[140px]" />
        <div className="absolute bottom-[-10%] left-[30%] w-[600px] h-[600px] bg-indigo-600/10 rounded-full blur-[140px]" />
      </div>

      {/* Persistent Navigation */}
      <Navbar currentPage={currentPage} onNavigate={handleNavigate} />

      {/* Routed Page Content */}
      <main className="flex-1 relative z-10">
        {currentPage === 'home' && <HomePage onNavigate={handleNavigate} />}
        {currentPage === 'playground' && <PlaygroundPage />}
        {currentPage === 'docs' && <DocsPage />}
        {currentPage === 'stdlib' && <StdlibPage />}
        {currentPage === 'architecture' && <ArchitecturePage />}
        {currentPage === 'benchmarks' && <BenchmarksPage />}
      </main>

      {/* Persistent Footer */}
      <Footer onNavigate={handleNavigate} />
    </div>
  );
}
