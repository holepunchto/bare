/* global bare */

/**
 * This module defines the global APIs available in Bare. In general, we prefer
 * modules over making APIs available in the global scope and mostly do the
 * latter to stay somewhat compatible with other environments, such as Web and
 * Node.js.
 */

/**
 * https://developer.mozilla.org/en-US/docs/Web/API/queueMicrotask
 */

const resolved = Promise.resolve()

const crash = err => setImmediate(() => { throw err })

global.queueMicrotask = function queueMicrotask (fn) {
  resolved
    .then(fn)
    // Make sure that exceptions are reported as normal uncaughts, not promise
    // rejections.
    .catch(crash)
}

/**
 * https://developer.mozilla.org/en-US/docs/Web/API/structuredClone
 */

global.structuredClone = require('bare-structured-clone')

/**
 * Thanks to Node.js, it is customary for the buffer API to be available
 * globally and many of the core modules in Bare also assume this.
 */

global.Buffer = require('bare-buffer')

/**
 * Make timers globally available, including the Node.js-specific immediate
 * API which schedules events to run after each I/O tick.
 */

const timers = require('bare-timers')

global.setTimeout = timers.setTimeout
global.clearTimeout = timers.clearTimeout

global.setInterval = timers.setInterval
global.clearInterval = timers.clearInterval

global.setImmediate = timers.setImmediate
global.clearImmediate = timers.clearImmediate

/**
 * Make the debugging console globally available.
 */

const Console = require('bare-console')

global.console = new Console({
  colors: bare.isTTY,
  bind: true,

  stdout (data) {
    bare.printInfo(data.replace(/\u0000/g, '\\x00')) // eslint-disable-line no-control-regex
  },

  stderr (data) {
    bare.printError(data.replace(/\u0000/g, '\\x00')) // eslint-disable-line no-control-regex
  }
})

/**
 * Make the URL constructor globally available.
 */

global.URL = require('bare-url').URL
