/* global pear */

const EventEmitter = require('./events')

module.exports = exports = class Thread extends EventEmitter {
  constructor (filename) {
    super()

    this._handle = pear.setupThread(filename)
  }

  join () {
    pear.joinThread(this._handle)
  }
}
