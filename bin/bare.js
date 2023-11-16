/* global Bare */
/* eslint-disable no-eval */
const Module = require('bare-module')
const path = require('bare-path')
const os = require('bare-os')

const argv = require('minimist')(Bare.argv.slice(1), {
  stopEarly: true,
  boolean: [
    'version',
    'help'
  ],
  string: [
    'import-map',
    'eval',
    'print'
  ],
  alias: {
    version: 'v',
    help: 'h',
    'import-map': 'm',
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
  console.log('usage: bare [-m, --import-map <path>] [<filename>]')

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
  const imports = Object.create(null)

  if (argv.m) {
    const { exports: map } = Module.load(
      Module.resolve(path.resolve(os.cwd(), argv.m))
    )

    if (map && map.imports) {
      for (const [from, to] of Object.entries(map.imports)) {
        if (/^([a-z]:)?([/\\]|\.{1,2}[/\\]?)/.test(to)) {
          imports[from] = path.resolve(os.cwd(), path.dirname(argv.m), to)
        } else {
          imports[from] = to
        }
      }
    }
  }

  Module.load(
    Bare.argv[1] = Module.resolve(
      path.resolve(os.cwd(), Bare.argv[1]),
      { imports }
    ),
    { imports }
  )
}
