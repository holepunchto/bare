const assert = require('assert')
const path = require('path')

const [
  pear,
  file
] = process.argv

assert(pear === path.resolve(process.cwd(), 'build/bin/pear'))
assert(file === path.resolve(process.cwd(), 'test/process-argv.js'))
