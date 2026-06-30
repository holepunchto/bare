const Module = require('bare-module')
const os = require('bare-os')
const url = require('bare-url')
const path = require('bare-path')
const Pipe = require('bare-pipe')
const Signal = require('bare-signals')
const { description, command, flag, arg, rest, bail } = require('paparam')

const { SIGUSR1 } = Signal.constants

const parentURL = url.pathToFileURL(os.cwd())

if (parentURL.pathname[parentURL.pathname.length - 1] !== '/') {
  parentURL.pathname += '/'
}

const channelFd = Number(os.getEnv('BARE_CHANNEL_FD'))
const channelMode = os.getEnv('BARE_CHANNEL_SERIALIZATION_MODE')

if (Number.isFinite(channelFd) && channelMode === 'binary') {
  Bare.IPC = new Pipe(channelFd, { ipc: true })
}

const bare = command(
  'bare',
  description('Evaluate a script or start a REPL session if no script is provided.'),
  flag('--version|-v', 'Print the Bare version'),
  flag('--eval|-e <script>', 'Evaluate an inline script'),
  flag('--print|-p <script>', 'Evaluate an inline script and print the result'),
  flag('--inspect', 'Activate the inspector'),
  flag('--inspect-port <port>', 'Configure the port on which the inspector will run').default(9229),
  flag('--expose-gc', 'Expose garbage collection APIs'),
  arg('[filename]', 'The name of a script to evaluate'),
  rest('[...args]', 'Additional arguments made available to the script'),
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
        return queueMicrotask(() => {
          throw bail.err
        })
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

    if (flags.version) return console.log(Bare.version)

    let server = null

    if (flags.inspect) inspect()
    else if (SIGUSR1) {
      const signal = new Signal(SIGUSR1)
      signal.unref()
      signal.on('signal', inspect).start()
    }

    const cache = Object.create(null)

    if (flags.eval) {
      return Module.load(parentURL, flags.eval, { cache })
    }

    if (flags.print) {
      return Module.load(parentURL, `console.log(${flags.print})`, { cache })
    }

    if (args.filename) {
      return Module.load(args.filename, { cache })
    }

    require('bare-repl')
      .start({ useGlobal: true })
      .on('exit', () => {
        if (server === null) Bare.exit()
        else server.close(() => Bare.exit())
      })

    function inspect() {
      if (server !== null) return

      const port = parseInt(flags.inspectPort)

      const inspector = require('bare-inspector')

      server = new inspector.Server(port, { path: args.filename || os.cwd() })
      server.unref()
    }
  }
)

bare.parse(Bare.argv.slice(1))
