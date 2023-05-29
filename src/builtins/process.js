/* global bare */

const EventEmitter = require('./events')

const resolved = Promise.resolve()

global.queueMicrotask = function queueMicrotask (fn) {
  resolved.then(fn)
}

class Process extends EventEmitter {
  get platform () {
    return bare.platform
  }

  get arch () {
    return bare.arch
  }

  get execPath () {
    return bare.execPath
  }

  get argv () {
    return bare.argv
  }

  get pid () {
    return bare.pid
  }

  get title () {
    return bare.getTitle()
  }

  set title (title) {
    if (typeof title === 'string') bare.setTitle(title)
  }

  get exitCode () {
    return bare.exitCode
  }

  set exitCode (code) {
    bare.exitCode = (Number(code) || 0) & 0xff
  }

  get versions () {
    return bare.versions
  }

  cwd () {
    return bare.cwd()
  }

  chdir (directory) {
    bare.chdir(directory)
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

  nextTick (cb, ...args) {
    queueMicrotask(cb.bind(null, ...args))
  }
}

global.process = module.exports = exports = new Process()

bare.onuncaughtexception = function onuncaughtexception (err) {
  if (exports.emit('uncaughtException', err)) return
  bare.printError(`Uncaught ${err.stack}\n`)
  bare.exitCode = 1
  bare.exit()
}

bare.onunhandledrejection = function onunhandledrejection (reason, promise) {
  if (exports.emit('unhandledRejection', reason, promise)) return
  bare.printError(`Uncaught (in promise) ${reason.stack}\n`)
  bare.exitCode = 1
  bare.exit()
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

exports.env = require('./process/env')
exports.addon = require('./process/addon')
exports.hrtime = require('./process/hrtime')
exports.thread = require('./process/thread')
