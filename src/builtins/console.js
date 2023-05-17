/* global bare */

const Console = require('bare-console')

global.console = module.exports = exports = new Console({
  stdout (data) {
    bare.printInfo(data.replace(/\u0000/g, '\\x00'))
  },
  stderr (data) {
    bare.printError(data.replace(/\u0000/g, '\\x00'))
  }
})

exports.Console = Console
