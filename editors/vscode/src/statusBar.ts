import * as vscode from 'vscode';
import type { LspStatus } from './lspClient';

/**
 * cpptools-style status bar: engine tag + IntelliSense/LSP health.
 */
export class RavenStatusBar implements vscode.Disposable {
    private readonly runItem: vscode.StatusBarItem;
    private readonly engineItem: vscode.StatusBarItem;
    private versionLabel = 'Raven';

    constructor() {
        this.runItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 100);
        this.runItem.command = 'raven.run';
        this.runItem.text = '$(play) Raven';
        this.runItem.tooltip = 'Run current Raven file';

        this.engineItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 99);
        this.engineItem.command = 'raven.restartLanguageServer';
        this.engineItem.text = '$(sync) Raven: starting';
        this.engineItem.tooltip = 'Restart Raven Language Server';
    }

    setVersion(version: string): void {
        this.versionLabel = version || 'Raven';
        this.runItem.text = `$(play) ${this.versionLabel}`;
    }

    setLspStatus(state: LspStatus, detail: string): void {
        switch (state) {
            case 'ready':
                this.engineItem.text = '$(check) Raven IntelliSense';
                this.engineItem.backgroundColor = undefined;
                break;
            case 'starting':
                this.engineItem.text = '$(sync~spin) Raven: starting';
                this.engineItem.backgroundColor = undefined;
                break;
            case 'error':
                this.engineItem.text = '$(error) Raven IntelliSense';
                this.engineItem.backgroundColor = new vscode.ThemeColor('statusBarItem.errorBackground');
                break;
            case 'stopped':
            default:
                this.engineItem.text = '$(circle-slash) Raven IntelliSense';
                this.engineItem.backgroundColor = undefined;
                break;
        }
        this.engineItem.tooltip = detail;
    }

    updateVisibility(editor: vscode.TextEditor | undefined): void {
        if (editor?.document.languageId === 'raven') {
            this.runItem.show();
            this.engineItem.show();
        } else {
            this.runItem.hide();
            this.engineItem.hide();
        }
    }

    dispose(): void {
        this.runItem.dispose();
        this.engineItem.dispose();
    }
}
