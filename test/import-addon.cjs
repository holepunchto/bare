const t = require('./harness')
const { Addon } = Bare

t.plan(1)

const mod = require(`./fixtures/addon/prebuilds/${Addon.host}/addon.bare`)

t.equal(mod, 'Hello from addon')
