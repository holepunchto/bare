/**
 * Step 0:
 * Declare the genesis addon entry point. All addons loaded by core modules
 * will pass through this function.
 */

const addons = Object.create(null)

bare.addon = function addon(href) {
  if (addons[href]) return addons[href]

  const addon = (addons[href] = { handle: null, exports: {} })

  addon.handle = bare.loadStaticAddon(href.replace(/^builtin:/, ''))

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

let suspending = false
let suspended = false
let exiting = false

class Bare extends EventEmitter {
  get platform() {
    return bare.platform
  }

  get arch() {
    return bare.arch
  }

  get simulator() {
    return bare.simulator
  }

  get argv() {
    return bare.argv
  }

  get pid() {
    return bare.pid
  }

  get exitCode() {
    return bare.exitCode
  }

  set exitCode(code) {
    bare.exitCode = code & 0xff
  }

  get suspending() {
    return suspending
  }

  get suspended() {
    return suspended
  }

  get exiting() {
    return exiting
  }

  get version() {
    return 'v' + bare.versions.bare
  }

  get versions() {
    return bare.versions
  }

  exit(code = bare.exitCode) {
    exiting = true

    bare.exitCode = code & 0xff
    bare.terminate()

    noop() // Trigger a stack check

    function noop() {}
  }

  suspend(linger = 0) {
    if (linger <= 0) linger = 0
    else linger = linger & 0xffffffff

    bare.suspend(linger)
  }

  idle() {
    bare.idle()
  }

  resume() {
    bare.resume()
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Bare },

      platform: this.platform,
      arch: this.arch,
      simulator: this.simulator,
      argv: this.argv,
      pid: this.pid,
      exitCode: this.exitCode,
      suspending: this.suspending,
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
 * Register the remaining global APIs. We prefer modules over making APIs
 * available in the global scope and do the latter to stay somewhat compatible
 * with other environments, such as Web and Node.js.
 */

require('bare-queue-microtask/global')
require('bare-buffer/global')
require('bare-timers/global')
require('bare-structured-clone/global')
require('bare-url/global')
require('bare-console/global')

/**
 * Step 7:
 * Register the native lifecycle hooks.
 */

bare.onuncaughtexception = function onuncaughtexception(err) {
  if (exiting || exports.emit('uncaughtException', err)) return

  try {
    console.error(`Uncaught %o`, err)
  } finally {
    bare.abort()
  }
}

bare.onunhandledrejection = function onunhandledrejection(reason, promise) {
  if (exiting || exports.emit('unhandledRejection', reason, promise)) return

  try {
    console.error(`Uncaught (in promise) %o`, reason)
  } finally {
    bare.abort()
  }
}

bare.onbeforeexit = function onbeforeexit() {
  exports.emit('beforeExit', bare.exitCode)
}

bare.onexit = function onexit() {
  exiting = true
  exports.emit('exit', bare.exitCode)
}

bare.onteardown = function onteardown() {
  for (const thread of exports.Thread._threads) {
    thread.join()
  }

  for (const addon of exports.Addon._addons) {
    addon.unload()
  }

  exports.emit('teardown')
}

bare.onsuspend = function onsuspend(linger) {
  suspending = true

  for (const thread of exports.Thread._threads) {
    thread.suspend(linger)
  }

  exports.emit('suspend', linger)
}

bare.onidle = function onidle() {
  suspending = false
  suspended = true

  exports.emit('idle')
}

bare.onresume = function onresume() {
  suspending = false
  suspended = false

  for (const thread of exports.Thread._threads) {
    thread.resume()
  }

  exports.emit('resume')
}

/**
 * Step 8:
 * Register the entry functions used by `bare_exit()` and `bare_load()`.
 */

const Module = require('bare-module')
const { startsWithWindowsDriveLetter } = require('bare-module-resolve')
const URL = require('bare-url')

bare.exit = exports.exit

bare.load = function load(filename, source) {
  let url

  if (startsWithWindowsDriveLetter(filename)) {
    url = null
  } else {
    url = URL.parse(filename)
  }

  if (url === null) url = URL.pathToFileURL(filename)

  return Module.load(url, source ? Buffer.from(source) : null)
}
