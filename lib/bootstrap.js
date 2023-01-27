/* global pear */

// overwritten by process, but in case things fail before...
pear.onfatalexception = err => pear.print(2, err.stack + '\n')

global.process = require('./process')
global.queueMicrotask = require('./queue-microtask')
global.Buffer = require('./buffer')

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

const timers = require('tiny-timers-native')

global.setTimeout = timers.setTimeout
global.clearTimeout = timers.clearTimeout

global.setInterval = timers.setInterval
global.clearInterval = timers.clearInterval

global.setImmediate = timers.setImmediate
global.clearImmediate = timers.clearImmediate

// bootstrap app now

pear.bootstrap = require('./module').bootstrap

process.bootstrapped = true
