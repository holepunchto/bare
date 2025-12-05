const t = require('./harness')

t.plan(1)

const mod = require('./fixtures/cjs')

t.equal(mod, 'Hello from CJS')
