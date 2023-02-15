/* global pear */

const Console = require('@pearjs/console')

global.console = module.exports = exports = new Console({
  stdout (data) {
    pear.print(1, data)
  },
  stderr (data) {
    pear.print(2, data)
  }
})

exports.Console = Console
