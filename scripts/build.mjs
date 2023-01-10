import childProcess from 'child_process'
import fs from 'fs/promises'
import esbuild from 'esbuild'
import includeStatic from 'include-static'

const result = await esbuild.build({
  entryPoints: [
    'src/bootstrap.js'
  ],
  bundle: true,
  minify: true,
  write: false,
  outfile: 'src/bootstrap.h'
})

for (const file of result.outputFiles) {
  await fs.writeFile(file.path, includeStatic('pearjs_bootstrap', Buffer.concat([file.contents, Buffer.from('\n//# sourceURL=<pearjs>/bootstrap.js')])))
}

childProcess.spawnSync('cmake', ['-S', '.', '-B', 'build', '-DCMAKE_BUILD_TYPE=Release'], {
  stdio: 'inherit'
})

childProcess.spawnSync('cmake', ['--build', 'build'], {
  stdio: 'inherit'
})
