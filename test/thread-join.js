/* global Bare */
const assert = require('bare-assert')
const { Thread } = Bare

assert(Thread.isMainThread === true)

const thread = new Thread(() => {})

thread.join()
