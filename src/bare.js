/* global bare */

/**
 * Step 1:
 * Load the core event emitter, which has no dependencies on the Bare namespace
 * or native code.
 */

const EventEmitter = require('bare-events')

/**
 * Step 2:
 * Declare the Bare object.
 */

class Bare extends EventEmitter {
  constructor () {
    super()

    this.suspended = false
    this.exiting = false
  }

  get platform () {
    return bare.platform
  }

  get arch () {
    return bare.arch
  }

  get argv () {
    return bare.argv
  }

  get pid () {
    return bare.pid
  }

  get exitCode () {
    return bare.exitCode
  }

  set exitCode (code) {
    bare.exitCode = code & 0xff
  }

  get version () {
    return 'v' + bare.versions.bare
  }

  get versions () {
    return bare.versions
  }

  exit (code = this.exitCode) {
    this.exiting = true
    this.exitCode = code

    bare.terminate()

    noop() // Trigger a stack check

    function noop () {}
  }

  suspend () {
    bare.suspend()
  }

  resume () {
    bare.resume()
  }

  _onuncaughtexception (err) {
    const inspect = require('bare-inspect')

    if (this.exiting || this.emit('uncaughtException', err)) return

    bare.printError(
      `Uncaught ${inspect(err, { colors: bare.isTTY })}\n`
    )

    bare.abort()
  }

  _onunhandledrejection (reason, promise) {
    const inspect = require('bare-inspect')

    if (this.exiting || this.emit('unhandledRejection', reason, promise)) return

    bare.printError(
      `Uncaught (in promise) ${inspect(reason, { colors: bare.isTTY })}\n`
    )

    bare.abort()
  }

  _onbeforeexit () {
    this.emit('beforeExit', bare.exitCode)
  }

  _onexit () {
    this.exiting = true

    this.emit('exit', bare.exitCode)
  }

  _onteardown () {
    this.emit('teardown')
  }

  _onsuspend () {
    this.suspended = true

    this.emit('suspend')
  }

  _onidle () {
    this.emit('idle')
  }

  _onresume () {
    this.suspended = false

    this.emit('resume')
  }

  [Symbol.for('bare.inspect')] () {
    return {
      __proto__: { constructor: Bare },

      platform: this.platform,
      arch: this.arch,
      argv: this.argv,
      pid: this.pid,
      exitCode: this.exitCode,
      suspended: this.suspended,
      exiting: this.exiting,
      version: this.version,
      versions: this.versions
    }
  }
}

/**
 * Step 3:
 * Construct the Bare namespace, after which other modules may access it using
 * the global `Bare` reference.
 */

module.exports = exports = new Bare()

Object.defineProperty(global, 'Bare', {
  value: exports,
  enumerable: true,
  writable: false,
  configurable: true
})

/**
 * Step 4:
 * Register the native addon API. Modules loaded from this point on may use
 * native code.
 */

const Addon = exports.Addon = require('./addon')

bare.addon = function addon (specifier) {
  let pkg

  switch (specifier) {
    case '/node_modules/bare-buffer':
      pkg = require('bare-buffer/package.json')
      break
    case '/node_modules/bare-timers':
      pkg = require('bare-timers/package.json')
      break
    case '/node_modules/bare-inspect':
      pkg = require('bare-inspect/package.json')
      break
    case '/node_modules/bare-hrtime':
      pkg = require('bare-hrtime/package.json')
      break
    case '/node_modules/bare-os':
      pkg = require('bare-os/package.json')
      break
    case '/node_modules/bare-url':
      pkg = require('bare-url/package.json')
      break
    case '/node_modules/bare-module':
      pkg = require('bare-module/package.json')
      break
    default:
      throw new Error(`Unknown addon '${specifier}'`)
  }

  const href = 'builtin:' + pkg.name + '@' + pkg.version

  if (Addon._cache[href]) return Addon._cache[href]._exports

  const addon = Addon._cache[href] = new Addon()

  addon._handle = bare.loadStaticAddon(pkg.name + '@' + pkg.version)

  addon._exports = bare.initAddon(addon._handle, addon._exports)

  return addon._exports
}

/**
 * Step 5:
 * Register the thread API.
 */

exports.Thread = require('./thread')

/**
 * Step 6:
 * Register the remaining globals.
 */

require('./globals')

/**
 * Step 7:
 * Register the native lifecycle hooks.
 */

bare.onuncaughtexception = exports._onuncaughtexception.bind(exports)
bare.onunhandledrejection = exports._onunhandledrejection.bind(exports)
bare.onbeforeexit = exports._onbeforeexit.bind(exports)
bare.onexit = exports._onexit.bind(exports)
bare.onteardown = exports._onteardown.bind(exports)
bare.onsuspend = exports._onsuspend.bind(exports)
bare.onidle = exports._onidle.bind(exports)
bare.onresume = exports._onresume.bind(exports)

/**
 * Step 8:
 * Register the main entry function used by `bare_run()`.
 */

bare.run = function run (filename, source) {
  const Module = require('bare-module')
  const url = require('bare-url')

  Module.load(url.pathToFileURL(filename), source)
}
