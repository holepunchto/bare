/* global pear */

const EventEmitter = require('../events')

class Thread extends EventEmitter {
  get data () {
    return pear.threadData === null ? null : Buffer.from(pear.threadData)
  }

  stop () {
    pear.stopCurrentThread()
  }
}

module.exports = pear.isMainThread ? null : new Thread()
