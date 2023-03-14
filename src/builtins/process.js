/* global pear */

const EventEmitter = require('./events')

const resolved = Promise.resolve()

global.queueMicrotask = function queueMicrotask (fn) {
  resolved.then(fn)
}

class Process extends EventEmitter {
  constructor () {
    super()

    this.errnos = new Map(pear.errnos)
  }

  get platform () {
    return pear.platform
  }

  get arch () {
    return pear.arch
  }

  get execPath () {
    return pear.execPath
  }

  get argv () {
    return pear.argv
  }

  get pid () {
    return pear.pid
  }

  get title () {
    return pear.getTitle()
  }

  set title (title) {
    if (typeof title === 'string') pear.setTitle(title)
  }

  get exitCode () {
    return pear.exitCode
  }

  set exitCode (code) {
    pear.exitCode = (Number(code) || 0) & 0xff
  }

  get versions () {
    return pear.versions
  }

  cwd () {
    return pear.cwd()
  }

  chdir (directory) {
    pear.chdir(directory)
  }

  exit (code = this.exitCode) {
    this.exitCode = code
    pear.exit()
  }

  suspend () {
    pear.suspend()
  }

  resume () {
    pear.resume()
  }

  nextTick (cb, ...args) {
    queueMicrotask(cb.bind(null, ...args))
  }
}

global.process = module.exports = exports = new Process()

pear.onuncaughtexception = function onuncaughtexception (err) {
  if (exports.emit('uncaughtException', err)) return
  pear.printError(`Uncaught ${err.stack}\n`)
  pear.exitCode = 1
  pear.exit()
}

pear.onunhandledrejection = function onunhandledrejection (reason, promise) {
  if (exports.emit('unhandledRejection', reason, promise)) return
  pear.printError(`Uncaught (in promise) ${reason.stack}\n`)
  pear.exitCode = 1
  pear.exit()
}

pear.onbeforeexit = function onbeforeexit () {
  exports.emit('beforeExit', pear.exitCode)
}

pear.onexit = function onexit () {
  exports.emit('exit', pear.exitCode)
}

pear.onsuspend = function onsuspend () {
  exports.emit('suspend')
}

pear.onidle = function onidle () {
  exports.emit('idle')
}

pear.onresume = function onresume () {
  exports.emit('resume')
}

exports.env = require('./process/env')
exports.data = require('./process/data')
exports.addon = require('./process/addon')
exports.hrtime = require('./process/hrtime')
