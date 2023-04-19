const path = require('path')
const Thread = require('thread')

const thread = new Thread(path.join(__dirname, 'fixtures/thread.cjs'))

thread.join()
