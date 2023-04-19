const path = require('path')
const Thread = require('thread')

const entry = path.join(__dirname, 'fixtures/thread.js')

const thread = new Thread(entry, Buffer.from('hello world'))

thread.join()
