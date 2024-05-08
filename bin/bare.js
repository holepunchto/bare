/* global Bare */
const Module = require('bare-module')
const os = require('bare-os')
const url = require('bare-url')
const { command, flag, arg, rest, bail } = require('paparam')

const parentURL = url.pathToFileURL(os.cwd())

if (parentURL.pathname[parentURL.pathname.length - 1] !== '/') {
  parentURL.pathname += '/'
}

const bare = command(
  'bare',
  flag('--version|-v', ''),
  flag('--eval|-e <script>', ''),
  flag('--print|-p <script>', ''),
  arg('<filename>', ''),
  rest('[...args]'),
  bail((reason) => queueMicrotask(() => { throw reason.err })),
  () => {
    const { args, flags, rest } = bare

    const argv = []

    if (args.filename) {
      args.filename = new URL(args.filename, parentURL)

      argv.push(url.fileURLToPath(args.filename))
    }

    if (rest) argv.push(...rest)

    Bare.argv.splice(1, Bare.argv.length - 1, ...argv)

    if (flags.version) {
      console.log(Bare.version)
    } else if (flags.eval) {
      Module.load(parentURL, `(${flags.eval})`)
    } else if (flags.print) {
      Module.load(parentURL, `console.log(${flags.print})`)
    } else if (args.filename) {
      Module.load(args.filename)
    } else {
      require('bare-repl').start()
    }
  }
)

bare.parse(Bare.argv.slice(1))
