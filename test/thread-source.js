const assert = require('assert')
const Thread = require('thread')

assert(Thread.isMainThread === true)

const entry = 'does-not-exist.js'

const thread = new Thread(entry, { source: Buffer.from('console.log(\'Hello from thread\')') })

thread.join()
