const assert = require('bare-assert')
const { Addon } = Bare

const mod = require(`./fixtures/addon/prebuilds/${Addon.host}/addon.bare`)

assert(mod === 'Hello from addon')
