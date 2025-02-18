const assert = require('bare-assert')
const path = require('bare-path')
const { Thread } = Bare

assert(Thread.isMainThread === true)

const entry = path.join(__dirname, 'fixtures/thread-shared-data.js')

const data = Buffer.from(new SharedArrayBuffer(4))

const thread = new Thread(entry, { data: data.buffer })

thread.join()

for (let i = 0; i < data.byteLength; i++) {
  assert(data[i] === i + 1)
}
