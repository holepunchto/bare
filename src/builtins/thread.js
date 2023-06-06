/* global bare */

module.exports = exports = class Thread {
  constructor (filename, opts = {}) {
    if (typeof filename === 'object') {
      opts = filename
      filename = '<thread>'
    }

    let {
      source = null,
      encoding = 'utf8',
      data = null,
      stackSize = 0
    } = opts

    if (typeof source === 'string') source = Buffer.from(source, encoding)

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
