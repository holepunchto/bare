/* global pear */

function onerror (err) {
  pear.print(2, err.stack + '\n')
}

pear.onunhandledrejection = pear.onuncaughtexception = onerror

global.process = require('./process')
global.Buffer = require('@pearjs/buffer')

const resolved = Promise.resolve()
global.queueMicrotask = function queueMicrotask (fn) {
  resolved.then(fn)
}

const Console = require('@pearjs/console')

global.console = new Console({
  stdout (data) {
    pear.print(1, data)
  },
  stderr (data) {
    pear.print(2, data)
  }
})

// setup timers

const timers = require('@pearjs/timers')

global.setTimeout = timers.setTimeout
global.clearTimeout = timers.clearTimeout

global.setInterval = timers.setInterval
global.clearInterval = timers.clearInterval

global.setImmediate = timers.setImmediate
global.clearImmediate = timers.clearImmediate

// bootstrap app now

pear.bootstrap = require('@pearjs/module').bootstrap

process.bootstrapped = true
