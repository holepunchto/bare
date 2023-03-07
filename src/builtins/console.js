/* global pear */

const Console = require('@pearjs/console')

global.console = module.exports = exports = new Console({
  stdout (data) {
    pear.printInfo(data)
  },
  stderr (data) {
    pear.printError(data)
  }
})

exports.Console = Console
