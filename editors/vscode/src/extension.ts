import { spawn } from 'child_process';
import * as path from 'path';
import * as vscode from 'vscode';
import { RavenLanguageClient } from './lspClient';
import { resolveRavenExecutable, resolveStdlibPath } from './ravenPath';
import { RavenStatusBar } from './statusBar';

let client: RavenLanguageClient | undefined;
let output: vscode.OutputChannel | undefined;
let statusBar: RavenStatusBar | undefined;

function getTerminal(name: string): vscode.Terminal {
    const existing = vscode.window.terminals.find((terminal) => terminal.name === name);
    if (existing) {
        return existing;
    }
    return vscode.window.createTerminal(name);
}

function getBreakpointsForFile(filePath: string): number[] {
    const lines: number[] = [];
    for (const breakpoint of vscode.debug.breakpoints) {
        if (breakpoint instanceof vscode.SourceBreakpoint &&
            breakpoint.location.uri.fsPath === filePath) {
            lines.push(breakpoint.location.range.start.line + 1);
        }
    }
    return lines;
}

function launchDebugger(config: {
    program?: string;
    stopOnEntry?: boolean;
    [key: string]: unknown;
} = {}): void {
    const editor = vscode.window.activeTextEditor;
    let filePath = config.program;
    if (!filePath || filePath === '${file}') {
        if (!editor) {
            void vscode.window.showErrorMessage('No active Raven file to debug.');
            return;
        }
        filePath = editor.document.uri.fsPath;
    }

    const ravenBin = resolveRavenExecutable();
    const breakpoints = getBreakpointsForFile(filePath);
    const bpArgs = breakpoints.map((line) => ` -b ${line}`).join('');
    const stopArg = config.stopOnEntry === false ? ' --no-stop-on-entry' : '';

    const terminal = getTerminal('Raven Debugger');
    terminal.show();
    terminal.sendText(`"${ravenBin}" debug "${filePath}"${bpArgs}${stopArg}`);
}

async function probeVersion(ravenBin: string): Promise<string> {
    return new Promise((resolve) => {
        const child = spawn(ravenBin, ['--version']);
        let stdout = '';
        child.stdout.on('data', (chunk: Buffer) => {
            stdout += chunk.toString();
        });
        child.on('error', () => resolve('Raven'));
        child.on('close', () => {
            const match = /Raven\s+([^\s]+)/.exec(stdout);
            resolve(match ? `Raven ${match[1]}` : 'Raven');
        });
    });
}

async function startLanguageServer(): Promise<void> {
    if (!client || !output || !statusBar) {
        return;
    }

    const config = vscode.workspace.getConfiguration('raven');
    if (config.get<string>('intelliSenseEngine') === 'Disabled') {
        client.stop();
        statusBar.setLspStatus('stopped', 'IntelliSense disabled in settings');
        return;
    }

    const ravenBin = resolveRavenExecutable();
    const version = await probeVersion(ravenBin);
    statusBar.setVersion(version);
    output.appendLine(`Using Raven executable: ${ravenBin}`);

    const stdlib = resolveStdlibPath(ravenBin);
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
    } catch (err) {
        const message = err instanceof Error ? err.message : String(err);
        output.appendLine(`Language server failed: ${message}`);
        void vscode.window.showWarningMessage(
            `Raven IntelliSense failed to start. Set raven.executablePath. (${message})`
        );
    }
}

export async function activate(context: vscode.ExtensionContext): Promise<void> {
    output = vscode.window.createOutputChannel('Raven');
    statusBar = new RavenStatusBar();
    client = new RavenLanguageClient(output);

    client.setStatusHandler((state, detail) => {
        statusBar?.setLspStatus(state, detail);
    });

    context.subscriptions.push(output, statusBar, client);

    output.appendLine('Raven Language Extension activated (TypeScript v1.1.0)');

    const updateStatusVisibility = (editor: vscode.TextEditor | undefined): void => {
        statusBar?.updateVisibility(editor);
    };
    updateStatusVisibility(vscode.window.activeTextEditor);
    context.subscriptions.push(
        vscode.window.onDidChangeActiveTextEditor(updateStatusVisibility)
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('raven.run', async () => {
            const editor = vscode.window.activeTextEditor;
            if (!editor) {
                void vscode.window.showErrorMessage('No active Raven file to run.');
                return;
            }
            await editor.document.save();
            const ravenBin = resolveRavenExecutable();
            const terminal = getTerminal('Raven');
            terminal.show();
            terminal.sendText(`"${ravenBin}" run "${editor.document.uri.fsPath}"`);
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('raven.runVM', async () => {
            const editor = vscode.window.activeTextEditor;
            if (!editor) {
                void vscode.window.showErrorMessage('No active Raven file to run in VM.');
                return;
            }
            await editor.document.save();
            const ravenBin = resolveRavenExecutable();
            const terminal = getTerminal('Raven VM');
            terminal.show();
            terminal.sendText(`"${ravenBin}" --vm "${editor.document.uri.fsPath}"`);
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('raven.repl', () => {
            const ravenBin = resolveRavenExecutable();
            const terminal = getTerminal('Raven REPL');
            terminal.show();
            terminal.sendText(`"${ravenBin}"`);
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('raven.disasm', async () => {
            const editor = vscode.window.activeTextEditor;
            if (!editor) {
                return;
            }
            await editor.document.save();
            const ravenBin = resolveRavenExecutable();
            const child = spawn(ravenBin, ['--disasm', editor.document.uri.fsPath]);
            let outputText = '';
            child.stdout.on('data', (chunk: Buffer) => { outputText += chunk.toString(); });
            child.stderr.on('data', (chunk: Buffer) => { outputText += chunk.toString(); });
            child.on('close', async () => {
                const doc = await vscode.workspace.openTextDocument({
                    content: outputText,
                    language: 'text'
                });
                await vscode.window.showTextDocument(doc, { preview: true, viewColumn: vscode.ViewColumn.Beside });
            });
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('raven.debug', () => {
            launchDebugger({ stopOnEntry: true });
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('raven.buildAndRun', async () => {
            const editor = vscode.window.activeTextEditor;
            if (!editor) {
                void vscode.window.showErrorMessage('No active Raven file to build.');
                return;
            }
            await editor.document.save();
            const filePath = editor.document.uri.fsPath;
            const ravenBin = resolveRavenExecutable();
            const outName = path.basename(filePath, path.extname(filePath));
            const terminal = getTerminal('Raven');
            terminal.show();
            if (process.platform === 'win32') {
                terminal.sendText(`& "${ravenBin}" build "${filePath}" -o "${outName}.exe"; if ($LASTEXITCODE -eq 0) { & ".\\${outName}.exe" }`);
            } else {
                terminal.sendText(`"${ravenBin}" build "${filePath}" -o "./${outName}" && "./${outName}"`);
            }
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('raven.fmt', async () => {
            const editor = vscode.window.activeTextEditor;
            if (!editor) {
                return;
            }
            await vscode.commands.executeCommand('editor.action.formatDocument');
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('raven.vet', () => {
            const editor = vscode.window.activeTextEditor;
            if (!editor || !output) {
                return;
            }
            const ravenBin = resolveRavenExecutable();
            output.clear();
            output.show(true);
            output.appendLine(`Analyzing ${editor.document.uri.fsPath} …\n`);
            const child = spawn(ravenBin, ['vet', editor.document.uri.fsPath]);
            child.stdout.on('data', (chunk: Buffer) => output?.append(chunk.toString()));
            child.stderr.on('data', (chunk: Buffer) => output?.append(chunk.toString()));
            child.on('close', (code) => {
                output?.appendLine(code === 0
                    ? '\n✓ Static analysis complete.'
                    : `\nAnalysis finished with exit code ${code}.`);
            });
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('raven.doc', () => {
            const editor = vscode.window.activeTextEditor;
            if (!editor) {
                return;
            }
            const ravenBin = resolveRavenExecutable();
            const child = spawn(ravenBin, ['doc', editor.document.uri.fsPath]);
            let markdown = '';
            child.stdout.on('data', (chunk: Buffer) => {
                markdown += chunk.toString();
            });
            child.on('close', async () => {
                const doc = await vscode.workspace.openTextDocument({
                    content: markdown,
                    language: 'markdown'
                });
                await vscode.window.showTextDocument(doc);
            });
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('raven.bench', () => {
            const editor = vscode.window.activeTextEditor;
            if (!editor) {
                return;
            }
            const ravenBin = resolveRavenExecutable();
            const terminal = getTerminal('Raven');
            terminal.show();
            terminal.sendText(`"${ravenBin}" bench "${editor.document.uri.fsPath}"`);
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('raven.restartLanguageServer', async () => {
            output?.appendLine('Restarting Raven Language Server…');
            await startLanguageServer();
            void vscode.window.showInformationMessage('Raven Language Server restarted.');
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('raven.selectExecutable', async () => {
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
            await vscode.workspace.getConfiguration('raven').update(
                'executablePath',
                picked[0].fsPath,
                vscode.ConfigurationTarget.Global
            );
            await startLanguageServer();
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('raven.editConfigurations', async () => {
            await vscode.commands.executeCommand(
                'workbench.action.openSettings',
                '@ext:raven-lang.raven-lang'
            );
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('raven.enableErrorSquiggles', async () => {
            await vscode.workspace.getConfiguration('raven').update(
                'errorSquiggles',
                'enabled',
                vscode.ConfigurationTarget.Global
            );
            client?.setSquigglesEnabled(true);
            await startLanguageServer();
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('raven.disableErrorSquiggles', async () => {
            await vscode.workspace.getConfiguration('raven').update(
                'errorSquiggles',
                'disabled',
                vscode.ConfigurationTarget.Global
            );
            client?.setSquigglesEnabled(false);
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('raven.logDiagnostics', () => {
            output?.show(true);
            output?.appendLine('--- Raven diagnostics dump ---');
            output?.appendLine(`executable: ${resolveRavenExecutable()}`);
            output?.appendLine(`stdlib: ${resolveStdlibPath(resolveRavenExecutable()) ?? '(none)'}`);
            output?.appendLine(`lsp ready: ${client?.isReady() ?? false}`);
            output?.appendLine(`platform: ${process.platform}`);
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('raven.rescanWorkspace', async () => {
            await startLanguageServer();
            void vscode.window.showInformationMessage('Raven workspace rescanned.');
        })
    );

    context.subscriptions.push(
        vscode.debug.registerDebugConfigurationProvider('raven', {
            provideDebugConfigurations(): vscode.DebugConfiguration[] {
                return [{
                    type: 'raven',
                    request: 'launch',
                    name: 'Raven: Debug Current File',
                    program: '${file}',
                    stopOnEntry: true
                }];
            },
            resolveDebugConfiguration(
                _folder: vscode.WorkspaceFolder | undefined,
                config: vscode.DebugConfiguration
            ): vscode.DebugConfiguration | undefined {
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
        })
    );

    context.subscriptions.push(
        vscode.languages.registerCompletionItemProvider(
            { language: 'raven' },
            {
                provideCompletionItems: async (document, position) =>
                    client?.completion(document, position) ?? []
            },
            '.',
            ':'
        )
    );

    context.subscriptions.push(
        vscode.languages.registerHoverProvider(
            { language: 'raven' },
            {
                provideHover: async (document, position) =>
                    client?.hover(document, position) ?? null
            }
        )
    );

    context.subscriptions.push(
        vscode.languages.registerDocumentFormattingEditProvider(
            { language: 'raven' },
            {
                provideDocumentFormattingEdits: async (document) =>
                    client?.formatting(document) ?? []
            }
        )
    );

    context.subscriptions.push(
        vscode.workspace.onDidOpenTextDocument((document) => client?.didOpen(document))
    );
    context.subscriptions.push(
        vscode.workspace.onDidChangeTextDocument((event) => {
            if (event.document.languageId === 'raven') {
                client?.didChange(event.document);
            }
        })
    );
    context.subscriptions.push(
        vscode.workspace.onDidCloseTextDocument((document) => client?.didClose(document))
    );

    context.subscriptions.push(
        vscode.workspace.onDidSaveTextDocument(async (document) => {
            if (document.languageId !== 'raven') {
                return;
            }
            const config = vscode.workspace.getConfiguration('raven');
            if (config.get<boolean>('formatOnSave')) {
                await vscode.commands.executeCommand('editor.action.formatDocument');
            }
        })
    );

    context.subscriptions.push(
        vscode.workspace.onDidChangeConfiguration(async (event) => {
            if (event.affectsConfiguration('raven')) {
                await startLanguageServer();
            }
        })
    );

    await startLanguageServer();
}

export function deactivate(): void {
    client?.dispose();
    client = undefined;
}
