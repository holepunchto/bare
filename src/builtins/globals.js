/**
 * This module defines the global APIs available in Bare. In general, we prefer
 * modules over making APIs available in the global scope and mostly to the
 * latter to stay somewhat compatible with other environments, such as Web and
 * Node.js.
 */

/**
 * https://developer.mozilla.org/en-US/docs/Web/API/queueMicrotask
 */

const resolved = Promise.resolve()

global.queueMicrotask = function queueMicrotask (fn) {
  resolved.then(fn)
}

/**
 * Thanks to Node.js, it is customary for the buffer API to be available
 * globally and many of the core modules in Bare also assume this.
 */

global.Buffer = require('./buffer')

/**
 *
 */

const timers = require('./timers')

global.setTimeout = timers.setTimeout
global.clearTimeout = timers.clearTimeout

global.setInterval = timers.setInterval
global.clearInterval = timers.clearInterval

global.setImmediate = timers.setImmediate
global.clearImmediate = timers.clearImmediate

/**
 *
 */

global.console = require('./console')
