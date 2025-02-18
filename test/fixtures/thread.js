const assert = require('bare-assert')
const { Thread } = Bare

assert(Thread.isMainThread === false)

assert(Thread.self.data.equals(Buffer.from('hello world')))
