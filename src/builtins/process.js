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
  get argv () {
    return bare.argv
  }

  get exitCode () {
    return bare.exitCode
  }

  set exitCode (code) {
    bare.exitCode = (Number(code) || 0) & 0xff
  }

  get suspended () {
    return bare.suspended
  }

  get version () {
    return 'v' + bare.versions.bare
  }

  get versions () {
    return bare.versions
  }

  exit (code = this.exitCode) {
    if (bare.isMainThread) {
      this.exitCode = code
      bare.exit()
    } else {
      bare.stopCurrentThread()
    }
  }

  suspend () {
    bare.suspend()
  }

  resume () {
    bare.resume()
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

    get execPath () {
      return os.execPath()
    }

    get pid () {
      return os.pid()
    }

    get ppid () {
      return os.ppid()
    }

    get title () {
      return os.getProcessTitle()
    }

    set title (title) {
      os.setProcessTitle(title)
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

const os = require('./os')

/**
 * Step 6:
 * Register the remaining globals.
 */

require('./globals')

/**
 * Step 7:
 * Register environment variable support and high-resolution timers.
 */

exports.env = require('bare-env')
exports.hrtime = require('bare-hrtime')

/**
 * Step 8:
 * Register the native hooks, propagating events to the process object.
 */

const inspect = require('bare-inspect')

bare.onuncaughtexception = function onuncaughtexception (err) {
  if (exports.emit('uncaughtException', err)) return

  bare.printError(`${`Uncaught ${inspect(err, { colors: bare.isTTY })}`}\n`)

  exports.exit(1)
}

bare.onunhandledrejection = function onunhandledrejection (reason, promise) {
  if (exports.emit('unhandledRejection', reason, promise)) return

  bare.printError(`${`Uncaught (in promise) ${inspect(reason, { colors: bare.isTTY })}`}\n`)

  exports.exit(1)
}

bare.onbeforeexit = function onbeforeexit () {
  exports.emit('beforeExit', bare.exitCode)
}

bare.onexit = function onexit () {
  exports.emit('exit', bare.exitCode)
}

bare.onsuspend = function onsuspend () {
  bare.suspended = true

  exports.emit('suspend')
}

bare.onidle = function onidle () {
  exports.emit('idle')
}

bare.onresume = function onresume () {
  bare.suspended = false

  exports.emit('resume')
}
