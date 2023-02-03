/* global pear */

const EventEmitter = require('@pearjs/events')

const process = module.exports = new EventEmitter()

pear.onexit = function onexit () {
  process.emit('exit')
}

pear.onbeforeexit = function onbeforeexit () {
  process.emit('beforeExit')
}

pear.onsuspend = function onsuspend () {
  process.emit('suspend')
}

pear.onresume = function () {
  process.emit('resume')
}

pear.onuncaughtexception = function onuncaughtexception (err) {
  if (process.emit('uncaughtException', err) === true) return
  console.error('Unhandled exception!')
  console.error(err.stack)
  pear.exit(1)
}

pear.onunhandledrejection = function unhandledRejection (reason, promise) {
  if (process.emit('unhandledRejection', reason, promise) === true) return
  console.error('Unhandled rejection!')
  console.error(reason.stack)
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

process.nextTick = function nextTick (cb, ...args) {
  const fn = cb.bind(null, ...args)
  queueMicrotask(fn)
}

process.addon = function addon (dirname, opts = {}) {
  if (typeof dirname !== 'string') throw new TypeError('dirname must be a string')

  const {
    resolve = false
  } = opts

  let mode = addon.dynamic ? pear.ADDONS_DYNAMIC : pear.ADDONS_STATIC

  if (resolve) mode |= pear.ADDONS_RESOLVE

  return pear.loadAddon(dirname, mode)
}

process.addon.dynamic = false

process.addon.resolve = function resolve (dirname) {
  if (typeof dirname !== 'string') throw new TypeError('dirname must be a string')

  return pear.resolveAddon(dirname)
}

const EMPTY = new Uint32Array([0, 0])

// TODO: a bit more safety on this one (ie validate prev)
process.hrtime = function hrtime (prev = EMPTY) {
  const result = new Uint32Array(2)
  pear.hrtime(result, prev)
  return result
}

process.hrtime.bigint = function () {
  const time = process.hrtime()
  return BigInt(time[0]) * BigInt(1e9) + BigInt(time[1])
}

function toExitCode (code) {
  return (Number(code) || 0) & 0xff
}
