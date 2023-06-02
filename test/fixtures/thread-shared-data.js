const assert = require('assert')
const Thread = require('thread')

assert(Thread.isMainThread === false)

const data = process.thread.data

for (let i = 0; i < data.byteLength; i++) {
  data[i] = i + 1
}
