const assert = require('assert')
const Addon = require('addon')
const build = require('./helpers/build.json')

assert(Addon.resolve(process.cwd()) === build.output.bare_addon)
