/* global pear */

const EventEmitter = require('../events')

class Thread extends EventEmitter {
  get data () {
    return Buffer.from(pear.threadData)
  }

  stop () {
    pear.stopCurrentThread()
  }
}

module.exports = pear.isMainThread ? null : new Thread()
