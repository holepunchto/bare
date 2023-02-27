const assert = require('assert')
const path = require('path')

const previous = process.cwd()

process.chdir('test')

console.log(process.cwd())

assert(process.cwd() === path.join(previous, 'test'))
