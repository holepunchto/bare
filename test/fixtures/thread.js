const assert = require('assert')
const Thread = require('thread')

assert(Thread.isMainThread === false)

assert(process.thread.data.equals(Buffer.from('hello world')))
