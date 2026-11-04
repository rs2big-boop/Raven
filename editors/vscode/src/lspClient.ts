import { ChildProcessWithoutNullStreams, spawn } from 'child_process';
import * as vscode from 'vscode';

export type LspStatus = 'stopped' | 'starting' | 'ready' | 'error';

interface PendingRequest {
    resolve: (value: unknown) => void;
    reject: (reason?: unknown) => void;
}

interface LspDiagnostic {
    range: {
        start: { line: number; character: number };
        end: { line: number; character: number };
    };
    severity?: number;
    message: string;
}

interface LspCompletionItem {
    label: string;
    kind?: number;
    detail?: string;
    documentation?: string;
}

/**
 * Minimal stdio JSON-RPC client for `raven lsp`.
 * Mirrors the cpptools pattern: one language engine process + editor glue.
 */
export class RavenLanguageClient implements vscode.Disposable {
    private process: ChildProcessWithoutNullStreams | null = null;
    private nextId = 1;
    private readonly pending = new Map<number, PendingRequest>();
    private buffer = Buffer.alloc(0);
    private contentLength = -1;
    private readonly diagnostics: vscode.DiagnosticCollection;
    private ready = false;
    private stopping = false;
    private onStatus: ((state: LspStatus, detail: string) => void) | null = null;

    constructor(private readonly output: vscode.OutputChannel) {
        this.diagnostics = vscode.languages.createDiagnosticCollection('raven');
    }

    setStatusHandler(handler: (state: LspStatus, detail: string) => void): void {
        this.onStatus = handler;
    }

    isReady(): boolean {
        return this.ready;
    }

    async start(command: string, args: string[] = ['lsp']): Promise<void> {
        this.stop();
        this.stopping = false;
        this.ready = false;
        this.setStatus('starting', 'Starting Raven Language Server…');

        try {
            this.process = spawn(command, args, { stdio: ['pipe', 'pipe', 'pipe'] });
        } catch (err) {
            const message = err instanceof Error ? err.message : String(err);
            this.output.appendLine(`Failed to spawn LSP: ${message}`);
            this.setStatus('error', message);
            throw err;
        }

        this.process.stdout.on('data', (chunk: Buffer) => this.onData(chunk));
        this.process.stderr.on('data', (chunk: Buffer) => {
            this.output.appendLine(`[lsp stderr] ${chunk.toString()}`);
        });
        this.process.on('error', (err: Error) => {
            this.output.appendLine(`LSP process error: ${err.message}`);
            this.ready = false;
            this.setStatus('error', err.message);
        });
        this.process.on('exit', (code, signal) => {
            this.ready = false;
            if (!this.stopping) {
                this.output.appendLine(`LSP exited (code=${code}, signal=${signal})`);
                this.setStatus('error', 'Language server stopped');
            }
            this.process = null;
        });

        await this.request('initialize', {
            processId: process.pid,
            clientInfo: { name: 'raven-vscode', version: '1.1.0' },
            capabilities: {
                textDocument: {
                    synchronization: { dynamicRegistration: false, didSave: true },
                    completion: { completionItem: { snippetSupport: true } },
                    hover: { contentFormat: ['markdown', 'plaintext'] },
                    formatting: { dynamicRegistration: false },
                    codeAction: { dynamicRegistration: false }
                },
                workspace: { workspaceFolders: true }
            },
            rootUri: vscode.workspace.workspaceFolders?.[0]?.uri.toString() ?? null,
            workspaceFolders: (vscode.workspace.workspaceFolders ?? []).map((folder) => ({
                uri: folder.uri.toString(),
                name: folder.name
            }))
        });

        this.notify('initialized', {});
        this.ready = true;
        this.setStatus('ready', 'IntelliSense ready');
        this.output.appendLine('Raven Language Server initialized.');
    }

    stop(): void {
        this.stopping = true;
        this.ready = false;

        for (const [, pending] of this.pending) {
            pending.reject(new Error('Language client stopped'));
        }
        this.pending.clear();
        this.diagnostics.clear();

        if (this.process) {
            try {
                this.notify('exit', null);
            } catch {
                // ignore
            }
            try {
                this.process.kill();
            } catch {
                // ignore
            }
            this.process = null;
        }

        this.setStatus('stopped', 'Language server stopped');
    }

    dispose(): void {
        this.stop();
        this.diagnostics.dispose();
    }

    didOpen(document: vscode.TextDocument): void {
        if (!this.ready || document.languageId !== 'raven') {
            return;
        }
        this.notify('textDocument/didOpen', {
            textDocument: {
                uri: document.uri.toString(),
                languageId: 'raven',
                version: document.version,
                text: document.getText()
            }
        });
    }

    didChange(document: vscode.TextDocument): void {
        if (!this.ready || document.languageId !== 'raven') {
            return;
        }
        this.notify('textDocument/didChange', {
            textDocument: {
                uri: document.uri.toString(),
                version: document.version
            },
            contentChanges: [{ text: document.getText() }]
        });
    }

    didClose(document: vscode.TextDocument): void {
        if (!this.ready || document.languageId !== 'raven') {
            return;
        }
        this.notify('textDocument/didClose', {
            textDocument: { uri: document.uri.toString() }
        });
        this.diagnostics.delete(document.uri);
    }

    async completion(
        document: vscode.TextDocument,
        position: vscode.Position
    ): Promise<vscode.CompletionItem[]> {
        if (!this.ready) {
            return [];
        }

        const result = await this.request('textDocument/completion', {
            textDocument: { uri: document.uri.toString() },
            position: { line: position.line, character: position.character }
        });

        const items = Array.isArray(result)
            ? (result as LspCompletionItem[])
            : (((result as { items?: LspCompletionItem[] } | null)?.items) ?? []);

        return items.map((item) => {
            const completion = new vscode.CompletionItem(item.label, this.completionKind(item.kind));
            completion.detail = item.detail;
            if (item.documentation) {
                completion.documentation = item.documentation;
            }
            return completion;
        });
    }

    async hover(
        document: vscode.TextDocument,
        position: vscode.Position
    ): Promise<vscode.Hover | null> {
        if (!this.ready) {
            return null;
        }

        const result = await this.request('textDocument/hover', {
            textDocument: { uri: document.uri.toString() },
            position: { line: position.line, character: position.character }
        }) as { contents?: unknown } | null;

        if (!result?.contents) {
            return null;
        }

        const contents = result.contents;
        let value = '';
        if (typeof contents === 'string') {
            value = contents;
        } else if (Array.isArray(contents)) {
            const first = contents[0] as { value?: string } | string | undefined;
            value = typeof first === 'string' ? first : (first?.value ?? '');
        } else if (typeof contents === 'object' && contents !== null && 'value' in contents) {
            value = String((contents as { value: unknown }).value);
        }

        return new vscode.Hover(new vscode.MarkdownString(value));
    }

    async formatting(document: vscode.TextDocument): Promise<vscode.TextEdit[]> {
        if (!this.ready) {
            return [];
        }

        const result = await this.request('textDocument/formatting', {
            textDocument: { uri: document.uri.toString() },
            options: { tabSize: 4, insertSpaces: true }
        });

        if (!Array.isArray(result)) {
            return [];
        }

        return (result as Array<{
            range: {
                start: { line: number; character: number };
                end: { line: number; character: number };
            };
            newText: string;
        }>).map((edit) => {
            const range = new vscode.Range(
                edit.range.start.line,
                edit.range.start.character,
                edit.range.end.line,
                edit.range.end.character
            );
            return vscode.TextEdit.replace(range, edit.newText);
        });
    }

    setSquigglesEnabled(enabled: boolean): void {
        if (!enabled) {
            this.diagnostics.clear();
        }
    }

    private setStatus(state: LspStatus, detail: string): void {
        this.onStatus?.(state, detail);
    }

    private request(method: string, params: unknown): Promise<unknown> {
        if (!this.process?.stdin.writable) {
            return Promise.reject(new Error('Language server is not running'));
        }

        const id = this.nextId++;
        const message = { jsonrpc: '2.0', id, method, params };

        return new Promise((resolve, reject) => {
            this.pending.set(id, { resolve, reject });
            this.send(message);
            setTimeout(() => {
                if (this.pending.has(id)) {
                    this.pending.delete(id);
                    reject(new Error(`LSP request timed out: ${method}`));
                }
            }, 15000);
        });
    }

    private notify(method: string, params: unknown): void {
        if (!this.process?.stdin.writable) {
            return;
        }
        this.send({ jsonrpc: '2.0', method, params });
    }

    private send(message: unknown): void {
        if (!this.process) {
            return;
        }
        const body = Buffer.from(JSON.stringify(message), 'utf8');
        const header = Buffer.from(`Content-Length: ${body.length}\r\n\r\n`, 'utf8');
        this.process.stdin.write(Buffer.concat([header, body]));
    }

    private onData(chunk: Buffer): void {
        this.buffer = Buffer.concat([this.buffer, chunk]);

        while (true) {
            if (this.contentLength < 0) {
                const headerEnd = this.buffer.indexOf('\r\n\r\n');
                if (headerEnd < 0) {
                    return;
                }

                const header = this.buffer.slice(0, headerEnd).toString('utf8');
                const match = /Content-Length:\s*(\d+)/i.exec(header);
                if (!match) {
                    this.buffer = this.buffer.slice(headerEnd + 4);
                    continue;
                }

                this.contentLength = Number.parseInt(match[1], 10);
                this.buffer = this.buffer.slice(headerEnd + 4);
            }

            if (this.buffer.length < this.contentLength) {
                return;
            }

            const body = this.buffer.slice(0, this.contentLength).toString('utf8');
            this.buffer = this.buffer.slice(this.contentLength);
            this.contentLength = -1;

            try {
                this.handleMessage(JSON.parse(body) as Record<string, unknown>);
            } catch (err) {
                const message = err instanceof Error ? err.message : String(err);
                this.output.appendLine(`Failed to parse LSP message: ${message}`);
            }
        }
    }

    private handleMessage(message: Record<string, unknown>): void {
        if (Object.prototype.hasOwnProperty.call(message, 'id') &&
            (Object.prototype.hasOwnProperty.call(message, 'result') ||
             Object.prototype.hasOwnProperty.call(message, 'error'))) {
            const id = message.id as number;
            const pending = this.pending.get(id);
            if (!pending) {
                return;
            }
            this.pending.delete(id);
            if (message.error) {
                const error = message.error as { message?: string };
                pending.reject(new Error(error.message || 'LSP error'));
            } else {
                pending.resolve(message.result);
            }
            return;
        }

        if (message.method === 'textDocument/publishDiagnostics') {
            this.publishDiagnostics(message.params as {
                uri?: string;
                diagnostics?: LspDiagnostic[];
            });
        }
    }

    private publishDiagnostics(params: { uri?: string; diagnostics?: LspDiagnostic[] }): void {
        if (!params?.uri) {
            return;
        }

        const config = vscode.workspace.getConfiguration('raven');
        if (config.get<string>('errorSquiggles') === 'disabled') {
            this.diagnostics.delete(vscode.Uri.parse(params.uri));
            return;
        }

        const diagnostics = (params.diagnostics ?? []).map((diagnostic) => {
            const range = new vscode.Range(
                diagnostic.range.start.line,
                diagnostic.range.start.character,
                diagnostic.range.end.line,
                diagnostic.range.end.character
            );
            const severity = diagnostic.severity === 1
                ? vscode.DiagnosticSeverity.Error
                : diagnostic.severity === 2
                    ? vscode.DiagnosticSeverity.Warning
                    : vscode.DiagnosticSeverity.Information;
            const item = new vscode.Diagnostic(range, diagnostic.message, severity);
            item.source = 'raven';
            return item;
        });

        this.diagnostics.set(vscode.Uri.parse(params.uri), diagnostics);
    }

    private completionKind(kind?: number): vscode.CompletionItemKind {
        switch (kind) {
            case 3:
                return vscode.CompletionItemKind.Function;
            case 6:
                return vscode.CompletionItemKind.Variable;
            case 7:
                return vscode.CompletionItemKind.Class;
            case 14:
                return vscode.CompletionItemKind.Keyword;
            default:
                return vscode.CompletionItemKind.Text;
        }
    }
}
