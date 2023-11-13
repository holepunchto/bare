const assert = require('bare-assert')

const { default: mod } = require('./fixtures/esm')

assert(mod === 'Hello from ESM')
