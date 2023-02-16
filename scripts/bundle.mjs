import path from 'path'
import url from 'url'
import dev from '@pearjs/dev'

const __filename = url.fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)

const cwd = path.join(__dirname, '..')

dev.bundle('/lib/pear.js', {
  cwd,
  protocol: 'pear',
  format: 'js',
  target: 'c',
  name: 'pear_bundle',
  header: '(function (pear) {',
  footer: '})',
  out: 'src/pear.js.h'
})
