module.exports = {
  byteLength,
  toString,
  write
}

function byteLength (string) {
  return string.length
}

function toString (buffer) {
  let string = ''

  for (let i = 0; i < buffer.byteLength; i++) {
    string += String.fromCharCode(buffer[i])
  }

  return string
}

function write (buffer, string, offset, length) {
  const len = Math.min(length, buffer.byteLength - offset)

  for (let i = 0; i < len; i++) {
    buffer[i] = string.charCodeAt(i)
  }

  return len
}
