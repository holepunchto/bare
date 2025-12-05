import t from 'bare-tap'
const { Addon } = Bare

t.plan(1)

const { default: mod } = await import(`./fixtures/addon/prebuilds/${Addon.host}/addon.bare`)

t.equal(mod, 'Hello from addon')
