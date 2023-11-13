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

  get argv () {
    return bare.argv
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
    if (this.exiting || this.emit('uncaughtException', err)) return

    bare.printError(
      `Uncaught ${inspect(err, { colors: bare.isTTY })}\n`
    )

    bare.abort()
  }

  _onunhandledrejection (reason, promise) {
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

      argv: this.argv,
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

global.Bare = module.exports = exports = new Bare()

/**
 * Step 4:
 * Register the native addon API. Modules loaded from this point on may use
 * native code.
 */

const Addon = exports.Addon = require('./addon')

bare.addon = Addon.load.bind(Addon)

/**
 * Step 5:
 * Now that native code is available, load the remaining core dependencies.
 */

const inspect = require('bare-inspect')
const Module = require('bare-module')
const path = require('bare-path')

/**
 * Step 6:
 * Register the thread API.
 */

exports.Thread = require('./thread')

/**
 * Step 7:
 * Register the remaining globals.
 */

require('./globals')

/**
 * Step 8:
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
 * Step 9:
 * Register the main entry function used by `bare_run()`.
 */

bare.run = function run (filename, source) {
  Module.load(path.normalize(filename), source)
}
