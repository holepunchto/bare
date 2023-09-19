/* global bare */

const EventEmitter = require('./events')

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

  get joined () {
    return this._joined
  }

  static create (filename, opts, callback) {
    return new Thread(filename, opts, callback)
  }

  join () {
    if (this._joined) return
    this._joined = true

    bare.joinThread(this._handle)
  }

  static get isMainThread () {
    return bare.isMainThread
  }

  [Symbol.for('bare.inspect')] () {
    return {
      __proto__: { constructor: Thread },

      joined: this.joined
    }
  }
}

class ThreadProxy extends EventEmitter {
  get data () {
    return ArrayBuffer.isView(bare.threadData) ? Buffer.coerce(bare.threadData) : bare.threadData
  }

  stop () {
    bare.stopCurrentThread()
  }

  [Symbol.for('bare.inspect')] () {
    return {
      __proto__: { constructor: ThreadProxy },

      data: this.data
    }
  }
}

exports.self = exports.isMainThread ? null : new ThreadProxy()
