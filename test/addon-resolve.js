/* global Bare */
const assert = require('bare-assert')
const path = require('bare-path')
const os = require('bare-os')
const build = require('./helpers/build.json')
const { Addon } = Bare

assert(Addon.resolve(os.cwd()) === path.normalize(build.output.bare_addon))
