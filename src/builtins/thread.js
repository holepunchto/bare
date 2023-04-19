/* global pear */

module.exports = exports = class Thread {
  constructor (filename, data, opts) {
    if (ArrayBuffer.isView(data)) {
      opts = opts || {}
    } else {
      opts = data || {}
      data = Buffer.alloc(0)
    }

    const {
      stackSize = 0
    } = opts

    this._joined = false

    this._handle = pear.setupThread(filename, data, stackSize)

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

  static get data () {
    return Buffer.from(pear.threadData)
  }
}

exports.isMainThread = pear.isMainThread
