const t = require('bare-tap')

t.plan(1)

const mod = require('./fixtures/cjs')

t.equal(mod, 'Hello from CJS')
