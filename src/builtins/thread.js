/* global bare */

module.exports = exports = class Thread {
  constructor (filename, opts = {}) {
    const {
      source = null,
      data = null,
      stackSize = 0
    } = opts

    this._joined = false

    this._handle = bare.setupThread(filename, source, data, stackSize)
  }

  join () {
    if (this._joined) return
    this._joined = true

    bare.joinThread(this._handle)
  }
}

exports.isMainThread = bare.isMainThread
