/* global bare, Bare */
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

    let { data = null, source = null, encoding = 'utf8', stackSize = 0 } = opts

    if (typeof source === 'string') source = Buffer.from(source, encoding)

    if (data !== null) {
      const serialized = structuredClone.serialize(data)

      const state = { start: 0, end: 0, buffer: null }

      structuredClone.preencode(state, serialized)

      data = new SharedArrayBuffer(state.end)

      state.buffer = Buffer.from(data)

      structuredClone.encode(state, serialized)
    }

    this._handle = bare.setupThread(filename, source, data, stackSize)

    Thread._threads.add(this)
  }

  get joined() {
    return this._handle === null
  }

  join() {
    if (this._handle) {
      bare.joinThread(this._handle)
      this._handle = null
    }

    Thread._threads.delete(this)
  }

  suspend() {
    if (this._handle) bare.suspendThread(this._handle)
  }

  resume() {
    if (this._handle) bare.resumeThread(this._handle)
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Thread },

      joined: this.joined
    }
  }

  static _threads = new Set()

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

  exports.self.data = structuredClone.deserialize(structuredClone.decode(state))
}

Bare.prependListener('teardown', () => {
  for (const thread of exports._threads) {
    thread.join()
  }
})
  .on('suspend', () => {
    for (const thread of exports._threads) {
      thread.suspend()
    }
  })
  .on('resume', () => {
    for (const thread of exports._threads) {
      thread.resume()
    }
  })
