/* global Bare */
const assert = require('bare-assert')
const { Thread } = Bare

assert(Thread.isMainThread === true)

const entry = 'does-not-exist.js'

const thread = new Thread(entry, {
  source: Buffer.from("console.log('Hello from thread')")
})

thread.join()
