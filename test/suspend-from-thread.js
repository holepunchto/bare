const Thread = require('thread')

process.on('suspend', () => process.resume())

const thread = new Thread(() => process.suspend())

thread.join()
