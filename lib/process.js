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

process.platform = pear.platform
process.arch = pear.arch
process.execPath = pear.execPath
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

process.cwd = function cwd () {
  return pear.cwd()
}

process.exit = function exit (code = 0) {
  if (typeof code !== 'number') code = 0
  pear.exit((code || 0) & 0xff)
}

process.addon = function addon (dirname, opts) {
  if (typeof dirname !== 'string') throw new TypeError('dirname must be a string')
  const resolve = (opts && opts.resolve) !== false
  return pear.loadAddon(dirname, resolve ? 1 : 0)
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
