/* global pear */

pear.onuncaughtexception = function onuncaughtexception (err) {
  pear.print(2, `Uncaught ${err.stack}\n`)
  pear.exit(1)
}

pear.onunhandledrejection = function unhandledRejection (reason, promise) {
  pear.print(2, `Uncaught (in promise) ${reason.stack}\n`)
  pear.exit(1)
}

const resolved = Promise.resolve()

global.queueMicrotask = function queueMicrotask (fn) {
  resolved.then(fn)
}

global.process = require('./process')

global.Buffer = require('@pearjs/buffer')

const Console = require('@pearjs/console')

global.console = new Console({
  stdout (data) {
    pear.print(1, data)
  },
  stderr (data) {
    pear.print(2, data)
  }
})

const timers = require('@pearjs/timers')

global.setTimeout = timers.setTimeout
global.clearTimeout = timers.clearTimeout

global.setInterval = timers.setInterval
global.clearInterval = timers.clearInterval

global.setImmediate = timers.setImmediate
global.clearImmediate = timers.clearImmediate

const Module = require('@pearjs/module')

Module.configure({
  exists (filename) {
    return pear.exists(filename)
  },

  read (filename) {
    return pear.read(filename)
  }
})

pear.bootstrap = Module.bootstrap

process.addon.dynamic = true
