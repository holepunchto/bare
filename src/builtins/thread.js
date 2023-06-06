/* global bare */

module.exports = exports = class Thread {
  constructor (filenameOrFunction, data, opts) {
    let filename

    if (Buffer.isBuffer(data) || data instanceof ArrayBuffer || data instanceof SharedArrayBuffer) {
      opts = opts || {}
    } else {
      opts = data || {}
      data = null
    }

    if (typeof filenameOrFunction === 'string') {
      filename = filenameOrFunction
    } else {
      filename = '<thread>'
      opts = { ...opts, source: `(${filenameOrFunction.toString()})(process.thread.data)` }
    }

    let {
      source = null,
      encoding = 'utf8',
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
