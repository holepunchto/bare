const assert = require('assert')

const { default: mod } = require('./fixtures/esm')

assert(mod === 'Hello from ESM')
