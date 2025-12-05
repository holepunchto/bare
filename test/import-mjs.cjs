const t = require('bare-tap')

t.plan(1)

const { default: mod } = require('./fixtures/esm')

t.equal(mod, 'Hello from ESM')
