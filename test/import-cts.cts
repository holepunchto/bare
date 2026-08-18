const t = require('bare-tap')

t.plan(1)

const mod: string = require('./fixtures/cts')

t.equal(mod, 'Hello from CTS')
