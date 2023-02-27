const assert = require('assert')
const path = require('path')

assert(process.addon.resolve(process.cwd()) === path.join(process.cwd(), 'build/pear_addon.pear'))
