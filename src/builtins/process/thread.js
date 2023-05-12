/* global bare */

const EventEmitter = require('../events')

class Thread extends EventEmitter {
  get data () {
    return bare.threadData === null ? null : Buffer.from(bare.threadData)
  }

  stop () {
    bare.stopCurrentThread()
  }
}

module.exports = bare.isMainThread ? null : new Thread()
