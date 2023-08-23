const Thread = require('thread')

process
  .on('suspend', () => {
    console.log('emit suspend')
  })
  .on('idle', () => {
    console.log('emit idle')
    new Thread(() => process.resume()) // eslint-disable-line no-new
  })
  .on('resume', () => {
    console.log('emit resume')
  })
  .suspend()
