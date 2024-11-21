/* global Bare */
const assert = require('bare-assert')
const url = require('bare-url')
const { Addon } = Bare

const addon = Addon.load(
  url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`)
)

assert(addon.exports === 'Hello from addon')
