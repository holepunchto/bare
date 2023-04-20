/* global pear */

module.exports = exports = class Thread {
  constructor (filename, opts = {}) {
    const {
      source = null,
      data = null,
      stackSize = 0
    } = opts

    this._joined = false

    this._handle = pear.setupThread(filename, source, data, stackSize)
  }

  join () {
    if (this._joined) return
    this._joined = true

    pear.joinThread(this._handle)
  }
}

exports.isMainThread = pear.isMainThread
