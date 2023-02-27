const assert = require('assert')

const addon = process.addon(process.cwd())

assert(addon === 'Hello from addon')
