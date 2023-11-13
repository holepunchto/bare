const assert = require('bare-assert')

const mod = require('./fixtures/cjs')

assert(mod === 'Hello from CJS')
