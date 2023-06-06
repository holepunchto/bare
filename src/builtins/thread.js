/* global bare */

module.exports = exports = class Thread {
  constructor (filenameOrFunction, opts = {}) {
    let filename

    // new Thread(options)
    if (typeof filenameOrFunction === 'object') {
      filename = '<thread>'
      opts = filenameOrFunction
    } 
    
    // new Thread(function[, options])
    else if (typeof filenameOrFunction === 'function') {
      filename = '<thread>'
      opts = {...opts, source: `(${filenameOrFunction.toString()})()` }
    } 
    
    // new Thread(filename[, options])
    else {
      filename = filenameOrFunction
    }

    let {
      source = null,
      encoding = 'utf8',
      data = null,
      stackSize = 0
    } = opts

    if (typeof source === 'string') source = Buffer.from(source, encoding)

    this._joined = false

    this._handle = bare.setupThread(filename, source, data, stackSize)
  }

  join () {
    if (this._joined) return
    this._joined = true

    bare.joinThread(this._handle)
  }
}

exports.isMainThread = bare.isMainThread
