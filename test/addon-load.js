/* global Bare */
const assert = require('bare-assert')
const os = require('bare-os')
const { Addon } = Bare

const addon = Addon.load(os.cwd())

assert(addon === 'Hello from addon')
