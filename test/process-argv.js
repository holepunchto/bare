const assert = require('assert')
const path = require('path')

const [
  bare,
  file
] = process.argv

assert(bare === path.resolve(process.cwd(), 'build/bin/bare'))
assert(file === path.resolve(process.cwd(), 'test/process-argv.js'))
