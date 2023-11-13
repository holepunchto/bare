/* global Bare */
const assert = require('bare-assert')
const { Thread } = Bare

assert(Thread.isMainThread === false)

const data = Buffer.from(Thread.self.data)

for (let i = 0; i < data.byteLength; i++) {
  data[i] = i + 1
}
