/* global Bare */
const Module = require('bare-module')
const os = require('bare-os')
const url = require('bare-url')
const path = require('bare-path')
const { command, flag, arg, rest, bail } = require('paparam')

const parentURL = url.pathToFileURL(os.cwd())

if (parentURL.pathname[parentURL.pathname.length - 1] !== '/') {
  parentURL.pathname += '/'
}

const bare = command(
  'bare',
  flag('--version|-v'),
  flag('--eval|-e <script>'),
  flag('--print|-p <script>'),
  arg('<filename>'),
  rest('[...args]'),
  bail((bail) => {
    switch (bail.reason) {
      case 'UNKNOWN_FLAG':
        console.error(`unknown flag: ${bail.flag.name}`)
        break

      case 'INVALID_FLAG':
        console.error(`invalid flag: ${bail.flag.name}`)
        break

      case 'UNKNOWN_ARG':
        console.error(`unknown argument: ${bail.arg.value}`)
        break

      default:
        return queueMicrotask(() => { throw bail.err })
    }

    Bare.exit(1)
  }),
  () => {
    const { args, flags, rest } = bare

    const argv = []

    if (args.filename) {
      args.filename = Module.resolve(path.resolve(args.filename), parentURL)

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
