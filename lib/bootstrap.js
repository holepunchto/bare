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

const Module = require('./module')

// bootstrap app now
process.main = Module.bootstrap(pear.main)
