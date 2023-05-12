const assert = require('assert')

const mod = require('../build/bare_addon.bare')

assert(mod === 'Hello from addon')
