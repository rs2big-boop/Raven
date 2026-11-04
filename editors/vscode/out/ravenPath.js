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
exports.resolveRavenExecutable = resolveRavenExecutable;
exports.resolveStdlibPath = resolveStdlibPath;
const fs = __importStar(require("fs"));
const os = __importStar(require("os"));
const path = __importStar(require("path"));
const vscode = __importStar(require("vscode"));
function exeName() {
    return process.platform === 'win32' ? 'raven.exe' : 'raven';
}
function exists(filePath) {
    try {
        return fs.existsSync(filePath);
    }
    catch {
        return false;
    }
}
/**
 * Resolve the Raven toolchain binary the same way cpptools resolves cl/clang/gcc:
 * settings first, then workspace build outputs, then user install dirs, then PATH.
 */
function resolveRavenExecutable() {
    const config = vscode.workspace.getConfiguration('raven');
    const configured = (config.get('executablePath') || '').trim();
    if (configured && exists(configured)) {
        return configured;
    }
    const name = exeName();
    const candidates = [];
    for (const folder of vscode.workspace.workspaceFolders ?? []) {
        const root = folder.uri.fsPath;
        candidates.push(path.join(root, 'build', 'Release', name), path.join(root, 'build', name), path.join(root, 'bin', name));
    }
    const home = os.homedir();
    candidates.push(path.join(home, '.raven', 'bin', name), path.join(home, '.local', 'bin', name), path.join('/usr/local/bin', name));
    if (process.platform === 'win32') {
        const localAppData = process.env.LOCALAPPDATA;
        if (localAppData) {
            candidates.push(path.join(localAppData, 'Programs', 'Raven', name));
        }
    }
    for (const candidate of candidates) {
        if (exists(candidate)) {
            return candidate;
        }
    }
    return name;
}
function resolveStdlibPath(ravenBin) {
    const config = vscode.workspace.getConfiguration('raven');
    const configured = (config.get('stdlibPath') || '').trim();
    if (configured && exists(configured)) {
        return configured;
    }
    const home = os.homedir();
    const guesses = [
        path.join(home, '.raven', 'stdlib'),
        path.join(home, '.local', 'share', 'raven', 'stdlib'),
        path.join('/usr/local/share/raven/stdlib')
    ];
    if (path.isAbsolute(ravenBin)) {
        const binDir = path.dirname(ravenBin);
        guesses.unshift(path.join(binDir, 'stdlib'), path.join(path.dirname(binDir), 'stdlib'), path.join(path.dirname(binDir), 'share', 'raven', 'stdlib'));
    }
    for (const folder of vscode.workspace.workspaceFolders ?? []) {
        guesses.unshift(path.join(folder.uri.fsPath, 'stdlib'));
    }
    for (const guess of guesses) {
        if (exists(guess)) {
            return guess;
        }
    }
    return undefined;
}
//# sourceMappingURL=ravenPath.js.map