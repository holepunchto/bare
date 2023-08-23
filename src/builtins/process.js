/* global bare */

/**
 * Step 1:
 * Load the core event emitter, which has no dependencies on the process object
 * or native code.
 */

const EventEmitter = require('./events')

/**
 * Step 2:
 * Declare the process object.
 */

class Process extends EventEmitter {
  get platform () {
    return bare.platform
  }

  get arch () {
    return bare.arch
  }

  get argv () {
    return bare.argv
  }

  get versions () {
    return bare.versions
  }

  get version () {
    return 'v' + bare.versions.bare
  }

  get exitCode () {
    return bare.exitCode
  }

  set exitCode (code) {
    bare.exitCode = (Number(code) || 0) & 0xff
  }

  get title () {
    return bare.getTitle()
  }

  set title (title) {
    if (typeof title === 'string') bare.setTitle(title)
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

  exit (code = this.exitCode) {
    this.exitCode = code
    bare.exit()
  }

  suspend () {
    bare.suspend()
  }

  resume () {
    bare.resume()
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

exports.addon = require('./process/addon')

/**
 * Step 5:
 * Register the thread API. Modules loaded from this point on may use threads,
 * including thread events for resource cleanup.
 */

exports.thread = require('./process/thread')

/**
 * Step 6:
 * Now that native code is available, load the builtin modules needed by the
 * process object.
 */

const os = require('./os')

/**
 * Step 7:
 * Register the remaining globals.
 */

require('./globals')

/**
 * Step 8:
 * Register environment variable support and high-resolution timers.
 */

exports.env = require('bare-env')
exports.hrtime = require('bare-hrtime')

/**
 * Step 9:
 * Register the native hooks, propagating events to the process object.
 */

bare.onuncaughtexception = function onuncaughtexception (err) {
  if (exports.emit('uncaughtException', err)) return

  bare.printError(`Uncaught ${err.stack}\n`)

  if (bare.isMainThread) {
    bare.exitCode = 1
    bare.exit()
  } else {
    bare.stopCurrentThread()
  }
}

bare.onunhandledrejection = function onunhandledrejection (reason, promise) {
  if (exports.emit('unhandledRejection', reason, promise)) return

  bare.printError(`Uncaught (in promise) ${reason.stack}\n`)

  if (bare.isMainThread) {
    bare.exitCode = 1
    bare.exit()
  } else {
    bare.stopCurrentThread()
  }
}

bare.onbeforeexit = function onbeforeexit () {
  exports.emit('beforeExit', bare.exitCode)
}

bare.onexit = function onexit () {
  exports.emit('exit', bare.exitCode)
}

bare.onthreadexit = function onthreadexit () {
  exports.thread.emit('exit')
}

bare.onsuspend = function onsuspend () {
  exports.emit('suspend')
}

bare.onidle = function onidle () {
  exports.emit('idle')
}

bare.onresume = function onresume () {
  exports.emit('resume')
}
