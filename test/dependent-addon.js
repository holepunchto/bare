/* global Bare */
const assert = require('bare-assert')
const url = require('bare-url')
const { Addon } = Bare

if (Bare.platform === 'win32') Bare.exit()

const a = Addon.load(url.pathToFileURL(`./test/fixtures/dependent-addon/a/prebuilds/${Addon.host}/a.bare`))
const b = Addon.load(url.pathToFileURL(`./test/fixtures/dependent-addon/b/prebuilds/${Addon.host}/b.bare`))

assert(a)
assert(b.exports === 42)
