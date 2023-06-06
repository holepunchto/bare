const Thread = require('thread')

process
  .on('idle', () =>
    new Thread({ source: 'process.resume()' })
  )
  .suspend()
