const assert = require('assert')
const Addon = require('addon')
const path = require('path')

assert(Addon.resolve(process.cwd()) === path.join(process.cwd(), 'build/bare_addon.bare'))
