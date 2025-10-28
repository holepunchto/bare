const structuredClone = require('bare-structured-clone')

module.exports = exports = class Thread {
  constructor(filename, opts, callback) {
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
      opts = {
        ...opts,
        source: `(${callback.toString()})(Bare.Thread.self.data)`
      }
    }

    let { data = null, source = null, encoding = 'utf8', stackSize = 0, transfer = [] } = opts

    if (typeof source === 'string') source = Buffer.from(source, encoding)

    if (data !== null) {
      const serialized = structuredClone.serializeWithTransfer(data, transfer)

      const state = { start: 0, end: 0, buffer: null }

      structuredClone.preencode(state, serialized)

      data = new SharedArrayBuffer(state.end)

      state.buffer = Buffer.from(data)

      structuredClone.encode(state, serialized)
    }

    this._handle = bare.setupThread(filename, source, data, stackSize)
  }

  get joined() {
    return this._handle === null
  }

  join() {
    if (this._handle) bare.joinThread(this._handle)

    this._handle = null
  }

  suspend(linger = 0) {
    if (linger <= 0) linger = 0
    else linger = linger & 0xffffffff

    if (this._handle) bare.suspendThread(this._handle, linger)
  }

  wakeup(deadline = 0) {
    if (deadline <= 0) deadline = 0
    else deadline = deadline & 0xffffffff

    if (this._handle) bare.wakeupThread(this._handle, deadline)
  }

  resume() {
    if (this._handle) bare.resumeThread(this._handle)
  }

  terminate() {
    if (this._handle) bare.terminateThread(this._handle)
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Thread },

      joined: this.joined
    }
  }

  static create(filename, opts, callback) {
    return new Thread(filename, opts, callback)
  }

  static get isMainThread() {
    return bare.isMainThread
  }
}

class ThreadProxy {
  constructor() {
    this.data = null
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: ThreadProxy },

      data: this.data
    }
  }
}

exports.self = exports.isMainThread ? null : new ThreadProxy()

bare.onthread = function onthread(data) {
  if (data === null) return

  const state = { start: 0, end: data.byteLength, buffer: Buffer.from(data) }

  exports.self.data = structuredClone.deserializeWithTransfer(structuredClone.decode(state))
}
