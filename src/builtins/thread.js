/* global bare */

module.exports = exports = class Thread {
  constructor (filename, opts, callback) {
    if (typeof filename === 'function') {
      callback = filename
      filename = '<thread>'
      opts = {}
    } else if (typeof filename === 'object') {
      callback = opts
      opts = filename
      filename = '<thread>'
    }

    if (typeof opts === 'function') {
      callback = opts
      opts = {}
    } else {
      opts = opts || {}
    }

    if (callback) {
      opts = { ...opts, source: `(${callback.toString()})(process.thread.data)` }
    }

    let {
      data = null,
      source = null,
      encoding = 'utf8',
      stackSize = 0
    } = opts

    if (typeof source === 'string') source = Buffer.from(source, encoding)

    this._joined = false

    this._handle = bare.setupThread(filename, source, data, stackSize)
  }

  static create (filename, opts, callback) {
    return new Thread(filename, opts, callback)
  }

  join () {
    if (this._joined) return
    this._joined = true

    bare.joinThread(this._handle)
  }
}

exports.isMainThread = bare.isMainThread
