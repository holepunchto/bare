/* global pear */

const hex = require('./hex')
const utf8 = require('./utf-8')
const base64 = require('./base64')
const ascii = require('./ascii')

module.exports = class Buffer extends Uint8Array {
  [Symbol.species] () {
    return Buffer
  }

  toString (encoding = 'utf-8', start = 0, end = this.byteLength) {
    return getEncoding(encoding).toString(subarrayMaybe(this, start, end))
  }

  write (string, offset = 0, length = this.byteLength - offset, encoding = 'utf-8') {
    if (typeof string !== 'string') throw new TypeError('string must be a string')
    return getEncoding(encoding).write(this, string, offset, length)
  }

  compare (target, targetStart = 0, targetEnd = target.byteLength, sourceStart = 0, sourceEnd = this.byteLength) {
    if (!(target instanceof Uint8Array)) throw new TypeError('target must be a buffer')
    return pear.bufferCompare(subarrayMaybe(this, sourceStart, sourceEnd), subarrayMaybe(target, targetStart, targetEnd))
  }

  copy (target, targetStart = 0, targetEnd = target.byteLength, sourceStart = 0, sourceEnd = this.byteLength) {
    if (!(target instanceof Uint8Array)) throw new TypeError('target must be a buffer')
    subarrayMaybe(this, sourceStart, sourceEnd).set(subarrayMaybe(target, targetStart, targetEnd))
    return Math.min(targetEnd - targetStart, sourceEnd - sourceStart)
  }

  equals (otherBuffer) {
    if (!(otherBuffer instanceof Uint8Array)) throw new TypeError('otherBuffer must be a buffer')
    return this === otherBuffer || pear.bufferCompare(this, otherBuffer) === 0
  }

  fill (value, offset = 0, end = this.byteLength, encoding = 'utf-8') {
    if (typeof value === 'number') {
      super.fill(value, offset, end)
      return this
    }

    if (offset < 0) offset = this.byteLength - offset

    const buffer = Buffer.from(value, encoding)

    while (offset < end) {
      if (end - offset < buffer.byteLength) {
        this.set(buffer.subarray(end - offset))
        break
      }
      this.set(buffer, offset)
      offset += buffer.byteLength
    }

    return this
  }

  static isBuffer (buffer) {
    return buffer instanceof Uint8Array
  }

  static byteLength (string, encoding = 'utf-8') {
    if (typeof string !== 'string') throw new TypeError('string must be a string')
    return getEncoding(encoding).byteLength(string)
  }

  static allocUnsafe (size) {
    if (typeof size !== 'number' || size < 0) throw new TypeError('size must be a positive number')
    pear.bufferSetZeroFillEnabled(0)
    try {
      return new Buffer(size)
    } finally {
      pear.bufferSetZeroFillEnabled(1)
    }
  }

  static alloc (size, fill, encoding) {
    const buf = new Buffer(size)
    if (fill) buf.fill(fill, 0, buf.byteLength, encoding)
    return buf
  }

  static compare (a, b) {
    if (!(a instanceof Uint8Array) || !(b instanceof Uint8Array)) throw new TypeError('a and b must be buffers')
    return pear.bufferCompare(a, b)
  }

  static from (val, a, b) {
    if (typeof val === 'string') {
      const encoding = getEncoding(a || 'utf-8')
      const buffer = Buffer.allocUnsafe(encoding.byteLength(val))
      encoding.write(buffer, val, 0, buffer.byteLength)
      return buffer
    }

    if (Array.isArray(val) || Buffer.isBuffer(val)) {
      const copy = Buffer.allocUnsafe(val.length)
      copy.set(val, 0)
      return copy
    }

    if (val instanceof ArrayBuffer) {
      return new Buffer(val, a, b)
    }

    throw new TypeError('Unsupported type passed to Buffer.from')
  }
}

function subarrayMaybe (buffer, start, end) {
  return start === 0 && end === buffer.byteLength ? buffer : buffer.subarray(start, end)
}

function getEncoding (name) {
  switch (name) {
    case 'utf8':
    case 'utf-8': return utf8
    case 'hex': return hex
    case 'ascii':
    case 'binary': return ascii
    case 'base64': return base64
  }

  throw new Error('Unsupported encoding, ' + name)
}
