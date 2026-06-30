const t = require('bare-tap')

t.plan(1)

const { default: mod } = require('./fixtures/mjs')

t.equal(mod, 'Hello from ESM')
