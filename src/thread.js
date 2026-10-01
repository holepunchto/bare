/* global bare */

const structuredClone = require('bare-structured-clone')

module.exports = exports = class Thread {
  constructor(filename, source, opts) {
    let callback = null

    if (typeof source === 'function') {
      callback = source
      source = null
    } else if (typeof opts === 'function') {
      callback = opts
      opts = source
      source = null
    }

    opts = opts || {}

    let { data = null, encoding = 'utf8', stackSize = 0, transfer = [] } = opts

    if (callback !== null) {
      source = `(${callback.toString()})(Bare.Thread.self.data)`
    }

    if (typeof source === 'string') {
      const copy = new SharedArrayBuffer(Buffer.byteLength(source, encoding))

      Buffer.from(copy).write(source, encoding)

      source = copy
    } else if (source !== null) {
      const copy = new SharedArrayBuffer(source.byteLength)

      Buffer.from(copy).set(source)

      source = copy
    }

    if (data !== null) {
      const serialized = structuredClone.serializeWithTransfer(data, transfer)

      const state = { start: 0, end: 0, buffer: null }

      structuredClone.preencode(state, serialized)

      data = new SharedArrayBuffer(state.end)

      state.buffer = Buffer.from(data)

      structuredClone.encode(state, serialized)
    }

    bare.setupThread(this, filename, source, data, stackSize)
  }

  get joined() {
    return bare.threadJoined(this)
  }

  join() {
    bare.joinThread(this)
  }

  suspend(linger = 0) {
    if (linger <= 0) linger = 0
    else linger = linger & 0xffffffff

    bare.suspendThread(this, linger)
  }

  wakeup(deadline = 0) {
    if (deadline <= 0) deadline = 0
    else deadline = deadline & 0xffffffff

    bare.wakeupThread(this, deadline)
  }

  resume() {
    bare.resumeThread(this)
  }

  terminate() {
    bare.terminateThread(this)
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Thread },

      joined: this.joined
    }
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
