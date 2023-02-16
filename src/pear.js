/* global pear */

require('./builtins/process')

const Module = require('./builtins/module')

pear.run = Module.load.bind(Module)
