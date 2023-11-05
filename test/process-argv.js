const assert = require('assert')
const path = require('path')
const build = require('./helpers/build.json')

const [
  bare,
  file
] = process.argv

assert(bare === build.output.bare)
assert(file === path.resolve(process.cwd(), 'test/process-argv.js'))
