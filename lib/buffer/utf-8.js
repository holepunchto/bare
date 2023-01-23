/* global pear */

module.exports = {
  byteLength,
  toString,
  write
}

function byteLength (string) {
  return pear.bufferByteLength(string)
}

function toString (buffer) {
  return pear.bufferToString(buffer)
}

function write (buffer, string, offset, length) {
  return pear.bufferWrite(buffer.subarray(offset, offset + length), string)
}
