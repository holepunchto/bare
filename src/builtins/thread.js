/* global pear */

const EventEmitter = require('./events')

module.exports = exports = class Thread extends EventEmitter {
  constructor (filename, opts = {}) {
    const {
      stackSize = 0
    } = opts

    super()

    this._handle = pear.setupThread(filename, stackSize)
  }

  join () {
    pear.joinThread(this._handle)
  }
}
