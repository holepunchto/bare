/* global pear */

const EventEmitter = require('@pearjs/events')

const process = global.process = module.exports = new EventEmitter()

pear.onbeforeexit = function onbeforeexit () {
  process.emit('beforeExit')
}

pear.onexit = function onexit () {
  process.emit('exit')
}

pear.onsuspend = function onsuspend () {
  process.emit('suspend')
}

pear.onresume = function onresume () {
  process.emit('resume')
}

pear.onuncaughtexception = function onuncaughtexception (err) {
  if (process.emit('uncaughtException', err)) return
  pear.print(2, `Uncaught ${err.stack}\n`)
  pear.exit(1)
}

pear.onunhandledrejection = function onunhandledrejection (reason, promise) {
  if (process.emit('unhandledRejection', reason, promise)) return
  pear.print(2, `Uncaught (in promise) ${reason.stack}\n`)
  pear.exit(1)
}

process.platform = pear.platform
process.arch = pear.arch
process.execPath = pear.execPath
process.argv = pear.argv
process.pid = pear.pid
process.versions = pear.versions

let env = null

Object.defineProperty(process, 'env', {
  get () {
    if (env === null) env = pear.env()
    return env
  },
  enumerable: true,
  configurable: false
})

process.errnos = new Map(pear.errnos)

process.cwd = function cwd () {
  return pear.cwd()
}

Object.defineProperty(process, 'title', {
  get () {
    return pear.getTitle()
  },
  set (title) {
    if (typeof title === 'string') pear.setTitle(title)
  },
  enumerable: true,
  configurable: false
})

Object.defineProperty(process, 'exitCode', {
  get () {
    return pear.exitCode
  },
  set (code) {
    pear.exitCode = toExitCode(code)
  },
  enumerable: true,
  configurable: false
})

process.exit = function exit (code = process.exitCode) {
  pear.exit(toExitCode(code))
}

process.suspend = function suspend () {
  pear.suspend()
}

process.resume = function resume () {
  pear.resume()
}

process.data = function data (key) {
  return pear.data[key] || null
}

const resolved = Promise.resolve()

global.queueMicrotask = function queueMicrotask (fn) {
  resolved.then(fn)
}

process.nextTick = function nextTick (cb, ...args) {
  queueMicrotask(cb.bind(null, ...args))
}

process.addon = require('./process/addon')

process.hrtime = require('./process/hrtime')

function toExitCode (code) {
  return (Number(code) || 0) & 0xff
}
