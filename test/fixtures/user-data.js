/* global callback */

const assert = require('assert')

const data = process.data('hello')

assert(data !== null)
callback(data)
