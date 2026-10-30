---
name: Bug Report
about: Create a report to help us improve Raven
title: '[BUG] '
labels: bug
assignees: ''
---

**Describe the Bug**
A clear and concise description of what the bug is.

**Raven Version**
Output of `raven --version`:

**Operating System & Architecture**
- OS: [e.g. Ubuntu 24.04, macOS Sonoma, Windows 11]
- Architecture: [e.g. x86_64, arm64]

**To Reproduce**
Steps or minimal `.rvn` code snippet to reproduce the behavior:
```raven
// Minimal reproducible example
fn main() {
    // ...
}
```

**Execution Command**
- [ ] Tree-Walk Interpreter: `raven run script.rvn`
- [ ] Corvus VM: `ravenc script.rvn -o script.rvc && rvm script.rvc`

**Expected Behavior**
A clear description of what you expected to happen.

**Actual Output & Stack Trace**
Paste the terminal output or stack trace here.
