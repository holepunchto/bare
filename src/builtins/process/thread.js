/* global bare */

const EventEmitter = require('../events')

class Thread extends EventEmitter {
  get data () {
    return ArrayBuffer.isView(bare.threadData) ? Buffer.coerce(bare.threadData) : bare.threadData
  }

  stop () {
    bare.stopCurrentThread()
  }
}

module.exports = bare.isMainThread ? null : new Thread()
