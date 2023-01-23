module.exports = {
  byteLength,
  toString,
  write
}

function byteLength (string) {
  return string.length >>> 1
}

function toString (buffer) {
  let s = ''

  for (let i = 0; i < buffer.byteLength; i++) {
    const h = buffer[i].toString(16)
    if (h.length === 1) s += '0' + h
    else s += h
  }

  return s
}

function write (buffer, string, offset, length) {
  const len = Math.min(length, buffer.byteLength - offset)

  for (let i = 0; i < len; i++) {
    buffer[offset + i] = 16 * getHalfHex(string, 2 * i) + getHalfHex(string, 2 * i + 1)
  }

  return len
}

function getHalfHex (s, i) {
  const n = s.charCodeAt(i)

  if (n >= 48 && n < 58) return n - 48
  if (n >= 65 && n < 71) return n - 65
  if (n >= 97 && n < 103) return n - 87

  throw new Error('Invalid hex string')
}
