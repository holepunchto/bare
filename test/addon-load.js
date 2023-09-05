const assert = require('assert')
const Addon = require('addon')

const addon = Addon.load(process.cwd())

assert(addon === 'Hello from addon')
