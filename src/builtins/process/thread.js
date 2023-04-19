/* global pear */

const EventEmitter = require('../events')

class Thread extends EventEmitter {
  stop () {
    pear.stopCurrentThread()
  }
}

module.exports = pear.isMainThread ? null : new Thread()
