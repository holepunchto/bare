const assert = require('assert')
const path = require('path')
const Thread = require('thread')

assert(Thread.isMainThread === true)

const entry = path.join(__dirname, 'fixtures/thread.js')

const thread = new Thread(entry, { data: Buffer.from('hello world') })

thread.join()
