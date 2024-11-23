/* global bare */

/**
 * This module defines the global APIs available in Bare. In general, we prefer
 * modules over making APIs available in the global scope and mostly do the
 * latter to stay somewhat compatible with other environments, such as Web and
 * Node.js.
 */

require('bare-queue-microtask/global')
require('bare-buffer/global')
require('bare-timers/global')
require('bare-structured-clone/global')
require('bare-url/global')

const Console = require('bare-console')

global.console = new Console({
  colors: bare.isTTY,
  bind: true,
  stdout(data) {
    bare.printInfo(data.replace(/\u0000/g, '\\x00'))
  },
  stderr(data) {
    bare.printError(data.replace(/\u0000/g, '\\x00'))
  }
})
