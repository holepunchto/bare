import path from 'path'
import url from 'url'
import childProcess from 'child_process'

const __filename = url.fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)

childProcess.spawnSync('git', ['submodule', 'update', '--init', '--recursive'], {
  stdio: 'inherit',
  cwd: path.join(__dirname, '..')
})
