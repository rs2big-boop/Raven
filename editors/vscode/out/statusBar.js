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
exports.RavenStatusBar = void 0;
const vscode = __importStar(require("vscode"));
/**
 * cpptools-style status bar: engine tag + IntelliSense/LSP health.
 */
class RavenStatusBar {
    constructor() {
        this.versionLabel = 'Raven';
        this.runItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 100);
        this.runItem.command = 'raven.run';
        this.runItem.text = '$(play) Raven';
        this.runItem.tooltip = 'Run current Raven file';
        this.engineItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 99);
        this.engineItem.command = 'raven.restartLanguageServer';
        this.engineItem.text = '$(sync) Raven: starting';
        this.engineItem.tooltip = 'Restart Raven Language Server';
    }
    setVersion(version) {
        this.versionLabel = version || 'Raven';
        this.runItem.text = `$(play) ${this.versionLabel}`;
    }
    setLspStatus(state, detail) {
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
    updateVisibility(editor) {
        if (editor?.document.languageId === 'raven') {
            this.runItem.show();
            this.engineItem.show();
        }
        else {
            this.runItem.hide();
            this.engineItem.hide();
        }
    }
    dispose() {
        this.runItem.dispose();
        this.engineItem.dispose();
    }
}
exports.RavenStatusBar = RavenStatusBar;
//# sourceMappingURL=statusBar.js.map