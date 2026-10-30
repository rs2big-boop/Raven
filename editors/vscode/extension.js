const vscode = require('vscode');
const { spawn } = require('child_process');
const path = require('path');
const fs = require('fs');

let lspProcess = null;
let outputChannel = null;

function getRavenPath() {
    const config = vscode.workspace.getConfiguration('raven');
    const customPath = config.get('executablePath');
    if (customPath && fs.existsSync(customPath)) {
        return customPath;
    }

    // Try workspace build/raven or /home/ran/Raven/build/raven or PATH
    const workspaceFolders = vscode.workspace.workspaceFolders;
    if (workspaceFolders) {
        for (const folder of workspaceFolders) {
            const localBin = path.join(folder.uri.fsPath, 'build', 'raven');
            if (fs.existsSync(localBin)) {
                return localBin;
            }
        }
    }

    const defaultDevPath = '/home/ran/Raven/build/raven';
    if (fs.existsSync(defaultDevPath)) {
        return defaultDevPath;
    }

    return 'raven';
}

function getBreakpointsForFile(filePath) {
    const bps = [];
    if (vscode.debug && vscode.debug.breakpoints) {
        for (const bp of vscode.debug.breakpoints) {
            if (bp instanceof vscode.SourceBreakpoint && bp.location && bp.location.uri) {
                if (bp.location.uri.fsPath === filePath) {
                    bps.push(bp.location.range.start.line + 1);
                }
            }
        }
    }
    return bps;
}

function launchDebuggerTerminal(config = {}) {
    const editor = vscode.window.activeTextEditor;
    let filePath = config.program;
    if (!filePath || filePath === '${file}') {
        if (!editor) {
            vscode.window.showErrorMessage("No active Raven file to debug.");
            return;
        }
        filePath = editor.document.uri.fsPath;
    }

    const ravenBin = getRavenPath();
    const breakpoints = getBreakpointsForFile(filePath);
    let bpArgs = "";
    for (const line of breakpoints) {
        bpArgs += ` -b ${line}`;
    }

    const stopOnEntry = config.stopOnEntry !== false;
    const stopArg = stopOnEntry ? "" : " --no-stop-on-entry";

    let terminal = vscode.window.terminals.find(t => t.name === 'Raven Debugger');
    if (!terminal) {
        terminal = vscode.window.createTerminal('Raven Debugger');
    }
    terminal.show();
    terminal.sendText(`"${ravenBin}" debug "${filePath}"${bpArgs}${stopArg}`);
}

function activate(context) {
    outputChannel = vscode.window.createOutputChannel("Raven Language");
    outputChannel.appendLine("Raven Language Extension Activated (v1.0.0)");

    // 1. Status Bar Item
    const statusBar = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 100);
    statusBar.command = 'raven.run';
    statusBar.text = '$(play) Raven Run';
    statusBar.tooltip = 'Execute current Raven file';
    context.subscriptions.push(statusBar);

    function updateStatusBar() {
        const editor = vscode.window.activeTextEditor;
        if (editor && editor.document.languageId === 'raven') {
            statusBar.show();
        } else {
            statusBar.hide();
        }
    }

    vscode.window.onDidChangeActiveTextEditor(updateStatusBar);
    updateStatusBar();

    // 2. Command: Raven Run File
    const runCmd = vscode.commands.registerCommand('raven.run', () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            vscode.window.showErrorMessage("No active Raven file to run.");
            return;
        }

        const filePath = editor.document.uri.fsPath;
        const ravenBin = getRavenPath();

        let terminal = vscode.window.activeTerminal;
        if (!terminal || terminal.name !== 'Raven') {
            terminal = vscode.window.createTerminal('Raven');
        }
        terminal.show();
        terminal.sendText(`"${ravenBin}" "${filePath}"`);
    });
    context.subscriptions.push(runCmd);

    // 2b. Command: Raven Debug File
    const debugCmd = vscode.commands.registerCommand('raven.debug', () => {
        launchDebuggerTerminal({ stopOnEntry: true });
    });
    context.subscriptions.push(debugCmd);

    // 2c. Register Debug Configuration Provider (F5 / Run & Debug View / launch.json)
    if (vscode.debug && vscode.debug.registerDebugConfigurationProvider) {
        context.subscriptions.push(vscode.debug.registerDebugConfigurationProvider('raven', {
            provideDebugConfigurations(folder, token) {
                return [
                    {
                        type: 'raven',
                        request: 'launch',
                        name: 'Raven: Debug Current File',
                        program: '${file}',
                        stopOnEntry: true
                    }
                ];
            },
            resolveDebugConfiguration(folder, config, token) {
                if (!config.type && !config.request && !config.name) {
                    const editor = vscode.window.activeTextEditor;
                    if (editor && editor.document.languageId === 'raven') {
                        config.type = 'raven';
                        config.name = 'Raven: Debug File';
                        config.request = 'launch';
                        config.program = '${file}';
                        config.stopOnEntry = true;
                    }
                }

                if (!config.program) {
                    vscode.window.showInformationMessage("Cannot find a Raven program to debug");
                    return undefined;
                }

                launchDebuggerTerminal(config);
                return undefined;
            }
        }));
    }

    // 3. Command: Raven Format File
    const fmtCmd = vscode.commands.registerCommand('raven.fmt', async () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) return;

        const filePath = editor.document.uri.fsPath;
        const ravenBin = getRavenPath();

        await editor.document.save();

        const child = spawn(ravenBin, ['fmt', filePath, '-w']);
        child.on('close', (code) => {
            if (code === 0) {
                vscode.window.showInformationMessage(`Formatted ${path.basename(filePath)}`);
            } else {
                vscode.window.showErrorMessage(`Failed to format ${path.basename(filePath)}`);
            }
        });
    });
    context.subscriptions.push(fmtCmd);

    // 4. Command: Raven Vet / Lint File
    const vetCmd = vscode.commands.registerCommand('raven.vet', () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) return;

        const filePath = editor.document.uri.fsPath;
        const ravenBin = getRavenPath();

        outputChannel.clear();
        outputChannel.show();
        outputChannel.appendLine(`Analyzing ${filePath} ...\n`);

        const child = spawn(ravenBin, ['vet', filePath]);
        child.stdout.on('data', (d) => outputChannel.append(d.toString()));
        child.stderr.on('data', (d) => outputChannel.append(d.toString()));
        child.on('close', (code) => {
            if (code === 0) {
                outputChannel.appendLine("\n✓ Static analysis complete.");
            }
        });
    });
    context.subscriptions.push(vetCmd);

    // 5. Command: Raven Doc Generator
    const docCmd = vscode.commands.registerCommand('raven.doc', () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) return;

        const filePath = editor.document.uri.fsPath;
        const ravenBin = getRavenPath();

        const child = spawn(ravenBin, ['doc', filePath]);
        let docMarkdown = '';
        child.stdout.on('data', (d) => docMarkdown += d.toString());
        child.on('close', async () => {
            const doc = await vscode.workspace.openTextDocument({
                content: docMarkdown,
                language: 'markdown'
            });
            vscode.window.showTextDocument(doc);
        });
    });
    context.subscriptions.push(docCmd);

    // 6. Command: Raven Bench
    const benchCmd = vscode.commands.registerCommand('raven.bench', () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) return;

        const filePath = editor.document.uri.fsPath;
        const ravenBin = getRavenPath();

        let terminal = vscode.window.activeTerminal;
        if (!terminal || terminal.name !== 'Raven') {
            terminal = vscode.window.createTerminal('Raven');
        }
        terminal.show();
        terminal.sendText(`"${ravenBin}" bench "${filePath}"`);
    });
    context.subscriptions.push(benchCmd);

    // 7. Format On Save Support
    vscode.workspace.onDidSaveTextDocument(async (doc) => {
        if (doc.languageId === 'raven') {
            const config = vscode.workspace.getConfiguration('raven');
            if (config.get('formatOnSave', false)) {
                vscode.commands.executeCommand('raven.fmt');
            }
        }
    });

    // 8. Start LSP Server
    const ravenBin = getRavenPath();
    try {
        lspProcess = spawn(ravenBin, ['lsp']);
        lspProcess.on('error', (err) => {
            outputChannel.appendLine(`LSP start error: ${err.message}`);
        });
    } catch (e) {
        outputChannel.appendLine(`Could not spawn LSP: ${e}`);
    }
}

function deactivate() {
    if (lspProcess) {
        lspProcess.kill();
    }
}

module.exports = {
    activate,
    deactivate
};
