/* global pear */

const EventEmitter = require('@pearjs/events')

global.process = module.exports = exports = new EventEmitter()

pear.onbeforeexit = function onbeforeexit () {
  exports.emit('beforeExit')
}

pear.onexit = function onexit () {
  exports.emit('exit')
}

pear.onsuspend = function onsuspend () {
  exports.emit('suspend')
}

pear.onresume = function onresume () {
  exports.emit('resume')
}

pear.onuncaughtexception = function onuncaughtexception (err) {
  if (exports.emit('uncaughtException', err)) return
  pear.print(2, `Uncaught ${err.stack}\n`)
  pear.exit(1)
}

pear.onunhandledrejection = function onunhandledrejection (reason, promise) {
  if (exports.emit('unhandledRejection', reason, promise)) return
  pear.print(2, `Uncaught (in promise) ${reason.stack}\n`)
  pear.exit(1)
}

exports.platform = pear.platform
exports.arch = pear.arch
exports.execPath = pear.execPath
exports.argv = pear.argv
exports.pid = pear.pid
exports.versions = pear.versions

let env = null

Object.defineProperty(exports, 'env', {
  get () {
    if (env === null) env = pear.env()
    return env
  },
  enumerable: true,
  configurable: false
})

exports.errnos = new Map(pear.errnos)

exports.cwd = function cwd () {
  return pear.cwd()
}

Object.defineProperty(exports, 'title', {
  get () {
    return pear.getTitle()
  },
  set (title) {
    if (typeof title === 'string') pear.setTitle(title)
  },
  enumerable: true,
  configurable: false
})

Object.defineProperty(exports, 'exitCode', {
  get () {
    return pear.exitCode
  },
  set (code) {
    pear.exitCode = toExitCode(code)
  },
  enumerable: true,
  configurable: false
})

exports.exit = function exit (code = exports.exitCode) {
  pear.exit(toExitCode(code))
}

exports.suspend = function suspend () {
  pear.suspend()
}

exports.resume = function resume () {
  pear.resume()
}

exports.data = function data (key) {
  return pear.data[key] || null
}

const resolved = Promise.resolve()

global.queueMicrotask = function queueMicrotask (fn) {
  resolved.then(fn)
}

exports.nextTick = function nextTick (cb, ...args) {
  queueMicrotask(cb.bind(null, ...args))
}

exports.addon = require('./exports/addon')

exports.hrtime = require('./exports/hrtime')

function toExitCode (code) {
  return (Number(code) || 0) & 0xff
}
