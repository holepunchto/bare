const Thread = require('thread')

process.on('suspend', () => process.resume())

const thread = new Thread({ source: 'process.suspend()' })

thread.join()
