const assert = require('bare-assert')
const path = require('bare-path')
const { Thread } = Bare

assert(Thread.isMainThread === true)

const entry = path.join(__dirname, 'fixtures/thread-transfer-data.js')

const buffer = Buffer.from('hello world')

const thread = new Thread(entry, { data: buffer, transfer: [buffer.buffer] })

thread.join()
