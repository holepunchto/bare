import path from 'path'
import url from 'url'
import childProcess from 'child_process'
import minimist from 'minimist'

const __filename = url.fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)

const root = path.join(__dirname, '..')

const argv = minimist(process.argv.slice(2), {
  boolean: [
    'debug'
  ]
})

const buildType = argv.debug ? 'Debug' : 'Release'

let proc = null

const args = ['-S', '.', '-B', 'build', `-DCMAKE_BUILD_TYPE=${buildType}`]

if (argv.debug) {
  args.push('-DPEAR_ENABLE_ASAN=ON')
}

proc = childProcess.spawnSync('cmake', args, {
  cwd: root,
  stdio: 'inherit'
})

if (proc.status) process.exit(proc.status)

proc = childProcess.spawnSync('cmake', ['--build', 'build'], {
  cwd: root,
  stdio: 'inherit'
})

if (proc.status) process.exit(proc.status)
