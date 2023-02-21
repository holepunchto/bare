const assert = require('assert')

const mod = require('./fixtures/bundle/mod')

assert(mod === 'Hello from bundle')
