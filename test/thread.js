const assert = require('bare-assert')
const path = require('bare-path')
const { Thread } = Bare

assert(Thread.isMainThread === true)

const entry = path.join(__dirname, 'fixtures/thread.js')

const thread = new Thread(entry, { data: Buffer.from('hello world') })

thread.join()
