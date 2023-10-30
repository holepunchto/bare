/* global bare */

require('./builtins/process')

const Module = require('./builtins/module')

const path = require('./builtins/path')

bare.run = function run (filename, source) {
  Module.load(path.normalize(filename), source)
}
