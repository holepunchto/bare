import childProcess from 'child_process'
import fs from 'fs/promises'
import path from 'path'
import url from 'url'
import esbuild from 'esbuild'
import includeStatic from 'include-static'

const __filename = url.fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)

const root = path.join(__dirname, '..')

const dirnamePlugin = {
  name: 'dirname-plugin',
  setup (build) {
    build.onLoad({ filter: /\.js$/, namespace: '' }, async (args) => {
      const src = await fs.readFile(args.path, 'utf-8')

      // const filename = path.resolve('/', path.relative(root, args.path))
      // const dirname = path.resolve('/', path.relative(root, path.dirname(args.path)))

      // TODO: name the anon paths when that works with libnapi etc
      const filename = args.path
      const dirname = path.dirname(args.path)

      return {
        contents: 'const __dirname = ' + JSON.stringify(dirname) + ', __filename = ' + JSON.stringify(filename) + ';' + src,
        loader: 'js'
      }
    })
  }
}

const result = await esbuild.build({
  plugins: [dirnamePlugin],
  entryPoints: [
    path.join(root, 'lib/bootstrap.js')
  ],
  alias: {
    'node-gyp-build': path.join(root, 'lib/load-addon.js')
  },
  bundle: true,
  write: false,
  outfile: path.join(root, 'build/bootstrap.js'),
  banner: {
    js: '(function (pear) {'
  },
  footer: {
    js: '})\n//# sourceURL=<pearjs>/bootstrap.js'
  }
})

for (const file of result.outputFiles) {
  await fs.mkdir(path.dirname(file.path), { recursive: true })

  await fs.writeFile(file.path, file.contents)
  await fs.writeFile(file.path.replace(/\.js$/, '.h'), includeStatic('pear_bootstrap', file.contents))
}

let proc = null

proc = childProcess.spawnSync('cmake', ['-S', '.', '-B', 'build', '-DCMAKE_BUILD_TYPE=Release'], {
  cwd: root,
  stdio: 'inherit'
})

if (proc.status) process.exit(proc.status)

proc = childProcess.spawnSync('cmake', ['--build', 'build'], {
  cwd: root,
  stdio: 'inherit'
})

if (proc.status) process.exit(proc.status)
