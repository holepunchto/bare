/* global pear */

const EventEmitter = require('@pearjs/events')

const process = module.exports = new EventEmitter()

pear.onexit = function () {
  process.emit('exit')
}

pear.onbeforeexit = function () {
  process.emit('beforeExit')
}

pear.onfatalexception = function (err) {
  if (process.emit('uncaughtException', err) === true) return
  console.error('Unhandled exception!')
  console.error(err.stack)
  pear.exit(1)
}

process.bootstrapped = false

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

process.nextTick = function nextTick (cb, ...args) {
  const fn = cb.bind(null, ...args)
  queueMicrotask(fn)
}

process.addon = function addon (dirname, opts) {
  if (typeof dirname !== 'string') throw new TypeError('dirname must be a string')
  const resolve = (opts && opts.resolve) !== false
  const static = (opts && opts.static) || !process.bootstrapped
  return pear.loadAddon(dirname, (static ?  pear.ADDONS_STATIC : pear.ADDONS_DYNAMIC) | (resolve ? pear.ADDONS_RESOLVE : 0))
}

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
