const assert = require('assert')
const Thread = require('thread')

assert(Thread.isMainThread === false)
assert(Thread.data.equals(Buffer.from('hello world')))
