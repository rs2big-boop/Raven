# Installing Raven

This guide covers building and installing Raven on Linux, macOS, and WSL.

---

## Prerequisites

Raven is written in clean, modern C++20 with minimal dependencies:
- **C++ Compiler**: GCC 11+ or Clang 13+ with C++20 support
- **Build System**: CMake 3.16+
- **Make / Ninja**: Standard build tools
- **Git**: For version control and package installation

On Debian/Ubuntu:
```bash
sudo apt update
sudo apt install -y build-essential cmake git
```

---

## Building from Source

1. **Clone the repository**:
   ```bash
   git clone https://github.com/raven-lang/raven.git
   cd raven
   ```

2. **Build the entire toolchain**:
   ```bash
   ./make.bash
   ```
   This compiles:
   - `raven`: Main executable (CLI toolchain, Interpreter, REPL, Debugger, LSP)
   - `ravenc`: Dedicated bytecode compiler
   - `rvm`: Corvus Virtual Machine runner

3. **Run the full test suite**:
   ```bash
   ./all.bash
   ```
   All 77 automated tests should pass.

---

## System-Wide Installation

To install Raven into `/usr/local/bin`:
```bash
sudo cp build/raven build/ravenc build/rvm /usr/local/bin/
sudo mkdir -p /usr/local/lib/raven
sudo cp -r stdlib /usr/local/lib/raven/
```

Verify installation:
```bash
raven --version
```

---

## Pre-Built Release Tarball

Download the standalone tarball:
```bash
tar -xzf raven-v1.0.0-linux-x86_64.tar.gz
cd raven-v1.0.0
./bin/raven --version
```

Verify SHA-256:
```bash
sha256sum -c raven-v1.0.0-linux-x86_64.tar.gz.sha256
```
