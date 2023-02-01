import path from 'path'
import url from 'url'
import minimist from 'minimist'
import dev from '@pearjs/dev'

const __filename = url.fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)

const cwd = path.join(__dirname, '..')

const argv = minimist(process.argv.slice(2), {
  boolean: [
    'debug'
  ]
})

dev.configure({ cwd, debug: argv.debug })

dev.build({ cwd })
