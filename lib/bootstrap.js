/* global pear */

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
const path = require('@pearjs/path')
const events = require('@pearjs/events')

Module._builtins.module = Module
Module._builtins.path = path
Module._builtins.timers = timers
Module._builtins.events = events
Module._builtins.buffer = { Buffer }

Module._protocols['file:'] = new Module.Protocol({
  exists (filename) {
    return pear.exists(filename)
  },

  read (filename) {
    return Buffer.from(pear.read(filename))
  }
})

pear.run = Module.load.bind(Module)
