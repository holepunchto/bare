/* global bare */

const Console = require('bare-console')

global.console = module.exports = exports = new Console({
  stdout (data) {
    bare.printInfo(data)
  },
  stderr (data) {
    bare.printError(data)
  }
})

exports.Console = Console
