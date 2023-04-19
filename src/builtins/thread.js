/* global pear */

const EventEmitter = require('./events')

module.exports = exports = class Thread extends EventEmitter {
  constructor (filename, opts = {}) {
    const {
      stackSize = 0
    } = opts

    super()

    this._joined = false

    this._handle = pear.setupThread(filename, stackSize)

    this._index = pear.threads.push(this) - 1
  }

  join () {
    if (this._joined) return
    this._joined = true

    pear.joinThread(this._handle)

    const head = pear.threads.pop()

    if (head !== this) {
      pear.threads[head._index = this._index] = head
    }
  }
}
