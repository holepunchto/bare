/* global Bare */
/* eslint-disable no-eval */
const Module = require('bare-module')
const path = require('bare-path')
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

if (argv.v) {
  console.log(Bare.version)

  Bare.exit()
}

if (argv.h) {
  console.log('usage: bare [<filename>]')

  Bare.exit(argv.h || argc > 0 ? 0 : 1)
}

if (argv.e) {
  eval(argv.e)

  Bare.exit()
}

if (argv.p) {
  console.log(eval(argv.p))

  Bare.exit()
}

if (argc === 0) {
  require('bare-repl').start()
} else {
  const resolved = new URL(Bare.argv[1], url.pathToFileURL(os.cwd() + path.sep))

  Bare.argv[1] = url.fileURLToPath(resolved)

  Module.load(resolved)
}
