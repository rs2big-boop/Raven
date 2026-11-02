import React, { useEffect } from 'react';
import { Routes, Route, useLocation } from 'react-router-dom';
import Navbar from './components/Navbar';
import Footer from './components/Footer';
import HomePage from './pages/HomePage';
import PlaygroundPage from './pages/PlaygroundPage';
import DocsPage from './pages/DocsPage';
import StdlibPage from './pages/StdlibPage';
import ArchitecturePage from './pages/ArchitecturePage';
import BenchmarksPage from './pages/BenchmarksPage';
import PackagesPage from './pages/PackagesPage';

function ScrollToTop() {
  const { pathname } = useLocation();
  useEffect(() => {
    window.scrollTo({ top: 0, behavior: 'smooth' });
  }, [pathname]);
  return null;
}

export default function App(): React.JSX.Element {
  return (
    <div className="min-h-screen bg-[#080a0f] text-slate-100 flex flex-col dev-grid-bg selection:bg-amber-500 selection:text-black">
      <ScrollToTop />
      {/* Persistent Navigation */}
      <Navbar />

      {/* Routed Page Content */}
      <main className="flex-1 relative z-10">
        <Routes>
          <Route path="/" element={<HomePage />} />
          <Route path="/playground" element={<PlaygroundPage />} />
          <Route path="/packages" element={<PackagesPage />} />
          <Route path="/docs" element={<DocsPage />} />
          <Route path="/stdlib" element={<StdlibPage />} />
          <Route path="/architecture" element={<ArchitecturePage />} />
          <Route path="/benchmarks" element={<BenchmarksPage />} />
          {/* Catch-all */}
          <Route path="*" element={<HomePage />} />
        </Routes>
      </main>

      {/* Persistent Footer */}
      <Footer />
    </div>
  );
}
