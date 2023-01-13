/* global pear */

// TODO: can we easily extend Uint8Arrays like in node?

const Buffer = module.exports = {}

Buffer.from = function from (str, enc) {
  if (enc && enc !== 'utf-8') throw new Error('Only Buffer.from utf-8 supported currently')
  const arr = pear.stringToBuffer(str)
  return new Uint8Array(arr, 0, arr.byteLength - 1)
}

Buffer.allocUnsafe = function allocUnsafe (n) {
  return new Uint8Array(n)
}

Buffer.alloc = function alloc (n) {
  return new Uint8Array(n)
}

Buffer.concat = function concat (arr, len) {
  if (typeof len !== 'number') {
    len = 0
    for (let i = 0; i < arr.length; i++) len += arr[i].byteLength
  }
  const result = Buffer.allocUnsafe(len)

  len = 0
  for (let i = 0; i < arr.length; i++) {
    result.set(arr[i], len)
    len += arr[i].byteLength
  }

  return result
}
