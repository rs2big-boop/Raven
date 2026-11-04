import * as fs from 'fs';
import * as os from 'os';
import * as path from 'path';
import * as vscode from 'vscode';

function exeName(): string {
    return process.platform === 'win32' ? 'raven.exe' : 'raven';
}

function exists(filePath: string): boolean {
    try {
        return fs.existsSync(filePath);
    } catch {
        return false;
    }
}

/**
 * Resolve the Raven toolchain binary the same way cpptools resolves cl/clang/gcc:
 * settings first, then workspace build outputs, then user install dirs, then PATH.
 */
export function resolveRavenExecutable(): string {
    const config = vscode.workspace.getConfiguration('raven');
    const configured = (config.get<string>('executablePath') || '').trim();
    if (configured && exists(configured)) {
        return configured;
    }

    const name = exeName();
    const candidates: string[] = [];

    for (const folder of vscode.workspace.workspaceFolders ?? []) {
        const root = folder.uri.fsPath;
        candidates.push(
            path.join(root, 'build', 'Release', name),
            path.join(root, 'build', name),
            path.join(root, 'bin', name)
        );
    }

    const home = os.homedir();
    candidates.push(
        path.join(home, '.raven', 'bin', name),
        path.join(home, '.local', 'bin', name),
        path.join('/usr/local/bin', name)
    );

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

export function resolveStdlibPath(ravenBin: string): string | undefined {
    const config = vscode.workspace.getConfiguration('raven');
    const configured = (config.get<string>('stdlibPath') || '').trim();
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
        guesses.unshift(
            path.join(binDir, 'stdlib'),
            path.join(path.dirname(binDir), 'stdlib'),
            path.join(path.dirname(binDir), 'share', 'raven', 'stdlib')
        );
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
