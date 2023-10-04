const assert = require('assert')
const Thread = require('thread')

assert(Thread.isMainThread === false)

assert(Thread.self.data.equals(Buffer.from('hello world')))
