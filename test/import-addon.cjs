const assert = require('assert')
const build = require('./helpers/build.json')

const mod = require(build.output.bare_addon)

assert(mod === 'Hello from addon')
