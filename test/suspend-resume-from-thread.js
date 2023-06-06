const Thread = require('thread')

process
  .on('idle', () =>
    new Thread(() => process.resume())
  )
  .suspend()
