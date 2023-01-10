import childProcess from 'child_process'
import fs from 'fs/promises'
import path from 'path'
import url from 'url'
import esbuild from 'esbuild'
import includeStatic from 'include-static'

const __filename = url.fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)

const result = await esbuild.build({
  entryPoints: [
    path.join(__dirname, '../src/bootstrap.js')
  ],
  bundle: true,
  minify: true,
  write: false,
  outfile: path.join(__dirname, '../src/bootstrap.h')
})

for (const file of result.outputFiles) {
  await fs.writeFile(file.path, includeStatic('pearjs_bootstrap', Buffer.concat([file.contents, Buffer.from('\n//# sourceURL=<pearjs>/bootstrap.js')])))
}

childProcess.spawnSync('cmake', ['-S', '.', '-B', 'build', '-DCMAKE_BUILD_TYPE=Release'], {
  cwd: path.join(__dirname, '..'),
  stdio: 'inherit'
})

childProcess.spawnSync('cmake', ['--build', 'build'], {
  cwd: path.join(__dirname, '..'),
  stdio: 'inherit'
})
