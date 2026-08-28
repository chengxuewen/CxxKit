#!/usr/bin/env node
// init-mcp-codegraph.mjs — CodeGraph MCP startup wrapper (cross-platform)
// Auto-detects Node.js environment, starts codegraph MCP server
import { execSync, spawn } from 'node:child_process'
import { existsSync } from 'node:fs'
import { join, dirname } from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = dirname(fileURLToPath(import.meta.url))
const projectRoot = join(__dirname, '..')
const isWin = process.platform === 'win32'

// 1. Locate codegraph binary
const binName = isWin ? 'codegraph.cmd' : 'codegraph'
const codegraphBin = join(projectRoot, 'node_modules', '.bin', binName)

// 2. Resolve a runnable node: the codegraph bin is a shell wrapper that does `exec node`,
//    so the pixi-managed node must be on PATH (system node may not exist, e.g. this machine).
function findNode() {
  const pixiNode = join(projectRoot, '.pixi', 'envs', 'default', 'bin', isWin ? 'node.exe' : 'node')
  if (existsSync(pixiNode)) return pixiNode
  return 'node' // fallback to PATH
}
const nodeBin = findNode()

// 3. Start MCP server via the shim JS directly (bypasses the wrapper's PATH-dependent `exec node`)
//    Shim search order: local node_modules → npx cache (shared with the main project's
//    update-codegraph.sh, which keeps vendored-repo indexes in sync).
function findShim() {
  const localShim = join(projectRoot, 'node_modules', '@colbymchenry', 'codegraph', 'npm-shim.js')
  if (existsSync(localShim)) return localShim
  const home = process.env.HOME ?? process.env.USERPROFILE ?? ''
  if (home) {
    const npxCache = join(home, '.npm', '_npx')
    try {
      const hit = execSync(`find "${npxCache}" -maxdepth 6 -path '*/@colbymchenry/codegraph/npm-shim.js' 2>/dev/null`, { encoding: 'utf8' }).split('\n')[0].trim()
      if (hit) return hit
    } catch {}
  }
  return ''
}
const npmShim = findShim()
if (!npmShim && !existsSync(codegraphBin)) {
  console.error('[codegraph] Installing @colbymchenry/codegraph...')
  const installCmd = existsSync(join(projectRoot, 'pnpm-lock.yaml'))
    ? 'pnpm add -wD @colbymchenry/codegraph'
    : 'npm install -D @colbymchenry/codegraph'
  execSync(installCmd, { cwd: projectRoot, stdio: 'inherit' })
}
const serverCmd = npmShim || codegraphBin
const child = spawn(nodeBin, [serverCmd, 'serve', '--mcp'], {
  stdio: 'inherit',
  env: { ...process.env, PATH: `${join(projectRoot, '.pixi', 'envs', 'default', 'bin')}:${process.env.PATH ?? ''}` },
})
child.on('error', (err) => { console.error(err); process.exit(1) })
child.on('exit', (code) => process.exit(code ?? 1))
