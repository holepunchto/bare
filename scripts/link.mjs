import path from 'path'
import url from 'url'
import fs from 'fs/promises'
import ScriptLinker from 'script-linker'
import includeStatic from 'include-static'

const __filename = url.fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)

const root = path.join(__dirname, '..')

const s = new ScriptLinker({
  bare: true,
  map (path) {
    return '<pear>' + path
  },
  mapResolve (req) {
    if (req === 'events') return '@pearjs/events'
    return req
  },
  readFile (filename) {
    return fs.readFile(path.join(root, filename))
  },
  builtins: {
    has () {
      return false
    },
    async get () {},
    keys () {
      return []
    }
  }
})

const bundle = await s.bundle('/lib/bootstrap.js', {
  header: '(function (pear) {',
  footer: '//# sourceURL=<pear>/bootstrap.js\n})'
})

await fs.writeFile(path.join(root, 'src/bootstrap.js'), bundle)

await fs.writeFile(path.join(root, 'src/bootstrap.js.h'), includeStatic('pear_bootstrap_js', Buffer.from(bundle)))
