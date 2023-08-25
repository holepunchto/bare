const assert = require('assert')
const Thread = require('thread')

assert(Thread.isMainThread === true)

const thread = new Thread(() => {})

thread.join()
