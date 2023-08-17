/* global bare */

const Console = require('bare-console')

global.console = module.exports = exports = new Console({
  colors: bare.isTTY,
  bind: true,

  stdout (data) {
    bare.printInfo(data.replace(/\u0000/g, '\\x00')) // eslint-disable-line no-control-regex
  },

  stderr (data) {
    bare.printError(data.replace(/\u0000/g, '\\x00')) // eslint-disable-line no-control-regex
  }
})

exports.Console = Console
