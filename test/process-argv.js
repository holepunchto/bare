const assert = require('assert')

const [
  pear,
  file
] = process.argv

assert(pear.endsWith('/build/bin/pear'))

assert(file === 'test/process-argv.js')
