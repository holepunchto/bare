/* global bare */

require('./builtins/process')

const Module = require('./builtins/module')

bare.run = Module.load.bind(Module)
