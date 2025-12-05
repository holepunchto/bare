const t = require('./harness')

t.plan(1)

const mod = require('./fixtures/bundle/mod.bundle')

t.equal(mod, 'Hello from bundle')
