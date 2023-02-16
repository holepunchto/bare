const assert = require('assert')

const mod = require('../build/pear_addon.pear')

assert(mod === 'Hello from addon')
