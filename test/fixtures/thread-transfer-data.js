const assert = require('bare-assert')
const { Thread } = Bare

assert(Thread.isMainThread === false)

const data = Thread.self.data

assert(data.toString() === 'hello world')
