import React, { useState } from 'react';
import Navbar from './components/Navbar';
import Footer from './components/Footer';
import HomePage from './pages/HomePage';
import PlaygroundPage from './pages/PlaygroundPage';
import DocsPage from './pages/DocsPage';
import StdlibPage from './pages/StdlibPage';
import ArchitecturePage from './pages/ArchitecturePage';
import BenchmarksPage from './pages/BenchmarksPage';
import PackagesPage from './pages/PackagesPage';

export default function App(): React.JSX.Element {
  const [currentPage, setCurrentPage] = useState<string>('home');

  // Scroll to top upon page navigation
  const handleNavigate = (pageId: string) => {
    setCurrentPage(pageId);
    window.scrollTo({ top: 0, behavior: 'smooth' });
  };

  return (
    <div className="min-h-screen bg-[#080a0f] text-slate-100 flex flex-col dev-grid-bg selection:bg-amber-500 selection:text-black">
      {/* Persistent Navigation */}
      <Navbar currentPage={currentPage} onNavigate={handleNavigate} />

      {/* Routed Page Content */}
      <main className="flex-1 relative z-10">
        {currentPage === 'home' && <HomePage onNavigate={handleNavigate} />}
        {currentPage === 'playground' && <PlaygroundPage />}
        {currentPage === 'packages' && <PackagesPage />}
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
