const assert = require('assert')

const mod = require('./fixtures/bundle/mod.bundle')

assert(mod === 'Hello from bundle')
