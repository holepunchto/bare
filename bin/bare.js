/* global Bare */
const Module = require('bare-module')
const os = require('bare-os')
const url = require('bare-url')

const argv = require('minimist')(Bare.argv.slice(1), {
  stopEarly: true,
  boolean: [
    'version',
    'help'
  ],
  string: [
    'eval',
    'print'
  ],
  alias: {
    version: 'v',
    help: 'h',
    eval: 'e',
    print: 'p'
  }
})

const argc = argv._.length

Bare.argv.splice(1, argc, ...argv._)

const parentURL = url.pathToFileURL(os.cwd())

if (parentURL.pathname[parentURL.pathname.length - 1] !== '/') {
  parentURL.pathname += '/'
}

if (argv.v) {
  console.log(Bare.version)
} else if (argv.h) {
  console.log('usage: bare [<filename>]')
} else if (argv.e) {
  Module.load(parentURL, `(${argv.e})`)
} else if (argv.p) {
  Module.load(parentURL, `console.log(${argv.p})`)
} else if (argc > 0) {
  const resolved = new URL(Bare.argv[1], parentURL)

  Bare.argv[1] = url.fileURLToPath(resolved)

  Module.load(resolved)
} else {
  require('bare-repl').start()
}
