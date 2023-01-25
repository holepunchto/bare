/* global pear */

const EventEmitter = require('@pearjs/events')

const process = module.exports = new EventEmitter()

pear.onexit = function () {
  process.emit('exit')
}

pear.onfatalexception = function (err) {
  if (process.emit('uncaughtException', err) === true) return
  console.error('Unhandled exception!')
  console.error(err.stack)
  pear.exit(1)
}

process.main = null // populated by bootstrap
process.bootstrapped = false

process.platform = pear.platform
process.arch = pear.arch
process.execPath = pear.execPath
process.argv = pear.argv
process.pid = pear.pid

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

process.exit = function exit (code = 0) {
  if (typeof code !== 'number') code = 0
  pear.exit((code || 0) & 0xff)
}

process.nextTick = function nextTick (cb, ...args) {
  const fn = cb.bind(null, ...args)
  queueMicrotask(fn)
}

process.addon = function addon (dirname, opts) {
  if (typeof dirname !== 'string') throw new TypeError('dirname must be a string')
  const resolve = (opts && opts.resolve) !== false
  return pear.loadAddon(dirname, (process.bootstrapped ? pear.ADDONS_DYNAMIC : pear.ADDONS_STATIC) | (resolve ? pear.ADDONS_RESOLVE : 0))
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
