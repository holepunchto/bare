/* global pear */

const Module = module.exports = require('@pearjs/module')

Module._builtins.module = Module

Module._builtins.buffer = require('./buffer')
Module._builtins.console = require('./console')
Module._builtins.events = require('./events')
Module._builtins.path = require('./path')
Module._builtins.process = require('./process')
Module._builtins.timers = require('./timers')

Module._protocols['file:'] = new Module.Protocol({
  exists (filename) {
    return pear.exists(filename)
  },

  read (filename) {
    return Buffer.from(pear.read(filename))
  }
})