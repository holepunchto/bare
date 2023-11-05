const assert = require('assert')
const Addon = require('addon')
const path = require('path')
const build = require('./helpers/build.json')

assert(Addon.resolve(process.cwd()) === path.normalize(build.output.bare_addon))
