"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
exports.activate = activate;
exports.deactivate = deactivate;
const child_process_1 = require("child_process");
const path = __importStar(require("path"));
const vscode = __importStar(require("vscode"));
const lspClient_1 = require("./lspClient");
const ravenPath_1 = require("./ravenPath");
const statusBar_1 = require("./statusBar");
let client;
let output;
let statusBar;
function getTerminal(name) {
    const existing = vscode.window.terminals.find((terminal) => terminal.name === name);
    if (existing) {
        return existing;
    }
    return vscode.window.createTerminal(name);
}
function getBreakpointsForFile(filePath) {
    const lines = [];
    for (const breakpoint of vscode.debug.breakpoints) {
        if (breakpoint instanceof vscode.SourceBreakpoint &&
            breakpoint.location.uri.fsPath === filePath) {
            lines.push(breakpoint.location.range.start.line + 1);
        }
    }
    return lines;
}
function launchDebugger(config = {}) {
    const editor = vscode.window.activeTextEditor;
    let filePath = config.program;
    if (!filePath || filePath === '${file}') {
        if (!editor) {
            void vscode.window.showErrorMessage('No active Raven file to debug.');
            return;
        }
        filePath = editor.document.uri.fsPath;
    }
    const ravenBin = (0, ravenPath_1.resolveRavenExecutable)();
    const breakpoints = getBreakpointsForFile(filePath);
    const bpArgs = breakpoints.map((line) => ` -b ${line}`).join('');
    const stopArg = config.stopOnEntry === false ? ' --no-stop-on-entry' : '';
    const terminal = getTerminal('Raven Debugger');
    terminal.show();
    terminal.sendText(`"${ravenBin}" debug "${filePath}"${bpArgs}${stopArg}`);
}
async function probeVersion(ravenBin) {
    return new Promise((resolve) => {
        const child = (0, child_process_1.spawn)(ravenBin, ['--version']);
        let stdout = '';
        child.stdout.on('data', (chunk) => {
            stdout += chunk.toString();
        });
        child.on('error', () => resolve('Raven'));
        child.on('close', () => {
            const match = /Raven\s+([^\s]+)/.exec(stdout);
            resolve(match ? `Raven ${match[1]}` : 'Raven');
        });
    });
}
async function startLanguageServer() {
    if (!client || !output || !statusBar) {
        return;
    }
    const config = vscode.workspace.getConfiguration('raven');
    if (config.get('intelliSenseEngine') === 'Disabled') {
        client.stop();
        statusBar.setLspStatus('stopped', 'IntelliSense disabled in settings');
        return;
    }
    const ravenBin = (0, ravenPath_1.resolveRavenExecutable)();
    const version = await probeVersion(ravenBin);
    statusBar.setVersion(version);
    output.appendLine(`Using Raven executable: ${ravenBin}`);
    const stdlib = (0, ravenPath_1.resolveStdlibPath)(ravenBin);
    if (stdlib) {
        output.appendLine(`Stdlib: ${stdlib}`);
    }
    try {
        await client.start(ravenBin, ['lsp']);
        for (const document of vscode.workspace.textDocuments) {
            if (document.languageId === 'raven') {
                client.didOpen(document);
            }
        }
    }
    catch (err) {
        const message = err instanceof Error ? err.message : String(err);
        output.appendLine(`Language server failed: ${message}`);
        void vscode.window.showWarningMessage(`Raven IntelliSense failed to start. Set raven.executablePath. (${message})`);
    }
}
async function activate(context) {
    output = vscode.window.createOutputChannel('Raven');
    statusBar = new statusBar_1.RavenStatusBar();
    client = new lspClient_1.RavenLanguageClient(output);
    client.setStatusHandler((state, detail) => {
        statusBar?.setLspStatus(state, detail);
    });
    context.subscriptions.push(output, statusBar, client);
    output.appendLine('Raven Language Extension activated (TypeScript v1.1.0)');
    const updateStatusVisibility = (editor) => {
        statusBar?.updateVisibility(editor);
    };
    updateStatusVisibility(vscode.window.activeTextEditor);
    context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(updateStatusVisibility));
    context.subscriptions.push(vscode.commands.registerCommand('raven.run', async () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            void vscode.window.showErrorMessage('No active Raven file to run.');
            return;
        }
        await editor.document.save();
        const ravenBin = (0, ravenPath_1.resolveRavenExecutable)();
        const terminal = getTerminal('Raven');
        terminal.show();
        terminal.sendText(`"${ravenBin}" run "${editor.document.uri.fsPath}"`);
    }));
    context.subscriptions.push(vscode.commands.registerCommand('raven.runVM', async () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            void vscode.window.showErrorMessage('No active Raven file to run in VM.');
            return;
        }
        await editor.document.save();
        const ravenBin = (0, ravenPath_1.resolveRavenExecutable)();
        const terminal = getTerminal('Raven VM');
        terminal.show();
        terminal.sendText(`"${ravenBin}" --vm "${editor.document.uri.fsPath}"`);
    }));
    context.subscriptions.push(vscode.commands.registerCommand('raven.repl', () => {
        const ravenBin = (0, ravenPath_1.resolveRavenExecutable)();
        const terminal = getTerminal('Raven REPL');
        terminal.show();
        terminal.sendText(`"${ravenBin}"`);
    }));
    context.subscriptions.push(vscode.commands.registerCommand('raven.disasm', async () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            return;
        }
        await editor.document.save();
        const ravenBin = (0, ravenPath_1.resolveRavenExecutable)();
        const child = (0, child_process_1.spawn)(ravenBin, ['--disasm', editor.document.uri.fsPath]);
        let outputText = '';
        child.stdout.on('data', (chunk) => { outputText += chunk.toString(); });
        child.stderr.on('data', (chunk) => { outputText += chunk.toString(); });
        child.on('close', async () => {
            const doc = await vscode.workspace.openTextDocument({
                content: outputText,
                language: 'text'
            });
            await vscode.window.showTextDocument(doc, { preview: true, viewColumn: vscode.ViewColumn.Beside });
        });
    }));
    context.subscriptions.push(vscode.commands.registerCommand('raven.debug', () => {
        launchDebugger({ stopOnEntry: true });
    }));
    context.subscriptions.push(vscode.commands.registerCommand('raven.buildAndRun', async () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            void vscode.window.showErrorMessage('No active Raven file to build.');
            return;
        }
        await editor.document.save();
        const filePath = editor.document.uri.fsPath;
        const ravenBin = (0, ravenPath_1.resolveRavenExecutable)();
        const outName = path.basename(filePath, path.extname(filePath));
        const terminal = getTerminal('Raven');
        terminal.show();
        if (process.platform === 'win32') {
            terminal.sendText(`& "${ravenBin}" build "${filePath}" -o "${outName}.exe"; if ($LASTEXITCODE -eq 0) { & ".\\${outName}.exe" }`);
        }
        else {
            terminal.sendText(`"${ravenBin}" build "${filePath}" -o "./${outName}" && "./${outName}"`);
        }
    }));
    context.subscriptions.push(vscode.commands.registerCommand('raven.fmt', async () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            return;
        }
        await vscode.commands.executeCommand('editor.action.formatDocument');
    }));
    context.subscriptions.push(vscode.commands.registerCommand('raven.vet', () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor || !output) {
            return;
        }
        const ravenBin = (0, ravenPath_1.resolveRavenExecutable)();
        output.clear();
        output.show(true);
        output.appendLine(`Analyzing ${editor.document.uri.fsPath} …\n`);
        const child = (0, child_process_1.spawn)(ravenBin, ['vet', editor.document.uri.fsPath]);
        child.stdout.on('data', (chunk) => output?.append(chunk.toString()));
        child.stderr.on('data', (chunk) => output?.append(chunk.toString()));
        child.on('close', (code) => {
            output?.appendLine(code === 0
                ? '\n✓ Static analysis complete.'
                : `\nAnalysis finished with exit code ${code}.`);
        });
    }));
    context.subscriptions.push(vscode.commands.registerCommand('raven.doc', () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            return;
        }
        const ravenBin = (0, ravenPath_1.resolveRavenExecutable)();
        const child = (0, child_process_1.spawn)(ravenBin, ['doc', editor.document.uri.fsPath]);
        let markdown = '';
        child.stdout.on('data', (chunk) => {
            markdown += chunk.toString();
        });
        child.on('close', async () => {
            const doc = await vscode.workspace.openTextDocument({
                content: markdown,
                language: 'markdown'
            });
            await vscode.window.showTextDocument(doc);
        });
    }));
    context.subscriptions.push(vscode.commands.registerCommand('raven.bench', () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            return;
        }
        const ravenBin = (0, ravenPath_1.resolveRavenExecutable)();
        const terminal = getTerminal('Raven');
        terminal.show();
        terminal.sendText(`"${ravenBin}" bench "${editor.document.uri.fsPath}"`);
    }));
    context.subscriptions.push(vscode.commands.registerCommand('raven.restartLanguageServer', async () => {
        output?.appendLine('Restarting Raven Language Server…');
        await startLanguageServer();
        void vscode.window.showInformationMessage('Raven Language Server restarted.');
    }));
    context.subscriptions.push(vscode.commands.registerCommand('raven.selectExecutable', async () => {
        const picked = await vscode.window.showOpenDialog({
            canSelectMany: false,
            openLabel: 'Select raven executable',
            filters: process.platform === 'win32'
                ? { Executable: ['exe'] }
                : undefined
        });
        if (!picked?.[0]) {
            return;
        }
        await vscode.workspace.getConfiguration('raven').update('executablePath', picked[0].fsPath, vscode.ConfigurationTarget.Global);
        await startLanguageServer();
    }));
    context.subscriptions.push(vscode.commands.registerCommand('raven.editConfigurations', async () => {
        await vscode.commands.executeCommand('workbench.action.openSettings', '@ext:raven-lang.raven-lang');
    }));
    context.subscriptions.push(vscode.commands.registerCommand('raven.enableErrorSquiggles', async () => {
        await vscode.workspace.getConfiguration('raven').update('errorSquiggles', 'enabled', vscode.ConfigurationTarget.Global);
        client?.setSquigglesEnabled(true);
        await startLanguageServer();
    }));
    context.subscriptions.push(vscode.commands.registerCommand('raven.disableErrorSquiggles', async () => {
        await vscode.workspace.getConfiguration('raven').update('errorSquiggles', 'disabled', vscode.ConfigurationTarget.Global);
        client?.setSquigglesEnabled(false);
    }));
    context.subscriptions.push(vscode.commands.registerCommand('raven.logDiagnostics', () => {
        output?.show(true);
        output?.appendLine('--- Raven diagnostics dump ---');
        output?.appendLine(`executable: ${(0, ravenPath_1.resolveRavenExecutable)()}`);
        output?.appendLine(`stdlib: ${(0, ravenPath_1.resolveStdlibPath)((0, ravenPath_1.resolveRavenExecutable)()) ?? '(none)'}`);
        output?.appendLine(`lsp ready: ${client?.isReady() ?? false}`);
        output?.appendLine(`platform: ${process.platform}`);
    }));
    context.subscriptions.push(vscode.commands.registerCommand('raven.rescanWorkspace', async () => {
        await startLanguageServer();
        void vscode.window.showInformationMessage('Raven workspace rescanned.');
    }));
    context.subscriptions.push(vscode.debug.registerDebugConfigurationProvider('raven', {
        provideDebugConfigurations() {
            return [{
                    type: 'raven',
                    request: 'launch',
                    name: 'Raven: Debug Current File',
                    program: '${file}',
                    stopOnEntry: true
                }];
        },
        resolveDebugConfiguration(_folder, config) {
            if (!config.type && !config.request && !config.name) {
                const editor = vscode.window.activeTextEditor;
                if (editor?.document.languageId === 'raven') {
                    config.type = 'raven';
                    config.name = 'Raven: Debug File';
                    config.request = 'launch';
                    config.program = '${file}';
                    config.stopOnEntry = true;
                }
            }
            if (!config.program) {
                void vscode.window.showInformationMessage('Cannot find a Raven program to debug');
                return undefined;
            }
            launchDebugger(config);
            return undefined;
        }
    }));
    context.subscriptions.push(vscode.languages.registerCompletionItemProvider({ language: 'raven' }, {
        provideCompletionItems: async (document, position) => client?.completion(document, position) ?? []
    }, '.', ':'));
    context.subscriptions.push(vscode.languages.registerHoverProvider({ language: 'raven' }, {
        provideHover: async (document, position) => client?.hover(document, position) ?? null
    }));
    context.subscriptions.push(vscode.languages.registerDocumentFormattingEditProvider({ language: 'raven' }, {
        provideDocumentFormattingEdits: async (document) => client?.formatting(document) ?? []
    }));
    context.subscriptions.push(vscode.workspace.onDidOpenTextDocument((document) => client?.didOpen(document)));
    context.subscriptions.push(vscode.workspace.onDidChangeTextDocument((event) => {
        if (event.document.languageId === 'raven') {
            client?.didChange(event.document);
        }
    }));
    context.subscriptions.push(vscode.workspace.onDidCloseTextDocument((document) => client?.didClose(document)));
    context.subscriptions.push(vscode.workspace.onDidSaveTextDocument(async (document) => {
        if (document.languageId !== 'raven') {
            return;
        }
        const config = vscode.workspace.getConfiguration('raven');
        if (config.get('formatOnSave')) {
            await vscode.commands.executeCommand('editor.action.formatDocument');
        }
    }));
    context.subscriptions.push(vscode.workspace.onDidChangeConfiguration(async (event) => {
        if (event.affectsConfiguration('raven')) {
            await startLanguageServer();
        }
    }));
    await startLanguageServer();
}
function deactivate() {
    client?.dispose();
    client = undefined;
}
//# sourceMappingURL=extension.js.map