/* global bare */

/**
 * Step 1:
 * Load the core event emitter, which has no dependencies on the process object
 * or native code.
 */

const EventEmitter = require('./events')

/**
 * Step 2:
 * Declare the process object, including the compatibility extensions mixin.
 * This mixin defines additional APIs that aren't part of the core process
 * object, but have been included for compatibility and convenience purposes.
 */

class Process extends WithCompatibilityExtensions(EventEmitter) {
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
      __proto__: { constructor: Process },

      platform: this.platform,
      arch: this.arch,
      title: this.title,
      pid: this.pid,
      ppid: this.ppid,

      argv: this.argv,
      exitCode: this.exitCode,
      suspended: this.suspended,
      exited: this.exited,
      version: this.version,
      versions: this.versions
    }
  }
}

function WithCompatibilityExtensions (Base) {
  return class extends Base {
    get platform () {
      return os.platform()
    }

    get arch () {
      return os.arch()
    }

    get title () {
      return os.getProcessTitle()
    }

    set title (title) {
      os.setProcessTitle(title)
    }

    get pid () {
      return os.pid()
    }

    get ppid () {
      return os.ppid()
    }

    get execPath () {
      return os.execPath()
    }

    get env () {
      return env
    }

    get hrtime () {
      return hrtime
    }

    cwd () {
      return os.cwd()
    }

    chdir (directory) {
      os.chdir(directory)
    }

    kill (pid, signal) {
      os.kill(pid, signal)
    }

    // For Node.js compatibility
    nextTick (cb, ...args) {
      queueMicrotask(cb.bind(null, ...args))
    }
  }
}

/**
 * Step 3:
 * Construct the process object, after which other modules may access it using
 * the global `process` reference.
 */

global.process = module.exports = exports = new Process()

/**
 * Step 4:
 * Register the native addon API. Modules loaded from this point on may use
 * native code.
 */

const Addon = require('./addon')

exports.addon = Addon.load.bind(Addon)

/**
 * Step 5:
 * Now that native code is available, load the builtin modules needed by the
 * process object.
 */

const os = require('bare-os')
const env = require('bare-env')
const hrtime = require('bare-hrtime')
const inspect = require('bare-inspect')

/**
 * Step 6:
 * Register the remaining globals.
 */

require('./globals')

/**
 * Step 7:
 * Register the native lifecycle hooks.
 */

bare.onuncaughtexception = process._onuncaughtexception.bind(process)
bare.onunhandledrejection = process._onunhandledrejection.bind(process)
bare.onbeforeexit = process._onbeforeexit.bind(process)
bare.onexit = process._onexit.bind(process)
bare.onteardown = process._onteardown.bind(process)
bare.onsuspend = process._onsuspend.bind(process)
bare.onidle = process._onidle.bind(process)
bare.onresume = process._onresume.bind(process)
