/* global Bare */
import assert from 'bare-assert'
const { Addon } = Bare

const { default: mod } = await import(`./fixtures/addon/prebuilds/${Addon.host}/addon.bare`)

assert(mod === 'Hello from addon')
