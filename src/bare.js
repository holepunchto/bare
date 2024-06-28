/* global bare */

/**
 * Step 0:
 * Declare the genesis addon entry point. All addons loaded by core modules
 * will pass through this function and the specifiers must therefore be
 * resolved statically without relying on any dependencies other than internal
 * bare.* APIs.
 */

const addons = Object.create(null)

bare.addon = function addon (specifier) {
  let pkg

  // Resolve the specifier to a known package manifest. The manifests must be
  // statically resolvable to allow them to be included by the bundler.
  switch (specifier) {
    case '/node_modules/bare-buffer/':
      pkg = require('bare-buffer/package')
      break
    case '/node_modules/bare-timers/':
      pkg = require('bare-timers/package')
      break
    case '/node_modules/bare-inspect/':
      pkg = require('bare-inspect/package')
      break
    case '/node_modules/bare-hrtime/':
      pkg = require('bare-hrtime/package')
      break
    case '/node_modules/bare-os/':
      pkg = require('bare-os/package')
      break
    case '/node_modules/bare-structured-clone/':
      pkg = require('bare-structured-clone/package')
      break
    case '/node_modules/bare-url/':
      pkg = require('bare-url/package')
      break
    case '/node_modules/bare-module/':
      pkg = require('bare-module/package')
      break
    case '/node_modules/bare-type/':
      pkg = require('bare-type/package')
      break
    default:
      throw new Error(`Unknown addon '${specifier}'`)
  }

  if (addons[pkg.name]) return addons[pkg.name]

  const addon = addons[pkg.name] = { handle: null, exports: {} }

  addon.handle = bare.loadStaticAddon(pkg.name + '@' + pkg.version)

  addon.exports = bare.initAddon(addon.handle, addon.exports)

  return addon.exports
}

/**
 * Step 1:
 * Load the core event emitter, which has no dependencies on the Bare namespace.
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

    this.exit = this.exit.bind(this)
  }

  get platform () {
    return bare.platform
  }

  get arch () {
    return bare.arch
  }

  get simulator () {
    return bare.simulator
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

  suspend (linger = 0) {
    bare.suspend(linger)
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

  _onsuspend (linger) {
    this.suspended = true

    this.emit('suspend', linger)
  }

  _onidle () {
    this.emit('idle')
  }

  _onresume () {
    this.suspended = false

    this.emit('resume')
  }

  _onthread (data) {
    exports.Thread.self._ondata(data)
  }

  [Symbol.for('bare.inspect')] () {
    return {
      __proto__: { constructor: Bare },

      platform: this.platform,
      arch: this.arch,
      simulator: this.simulator,
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
 * Register the native addon API.
 */

exports.Addon = require('./addon')

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
bare.onthread = exports._onthread.bind(exports)

/**
 * Step 8:
 * Register the entry functions used by `bare_exit()` and `bare_load()`.
 */

const Module = require('bare-module')
const url = require('bare-url')

bare.exit = exports.exit

bare.load = function load (filename, source) {
  return Module.load(url.pathToFileURL(filename), source ? Buffer.from(source) : null)
}
