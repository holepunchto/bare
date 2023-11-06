const Thread = require('thread')

process
  .on('suspend', () => {
    console.log('emit suspend')
    process.resume()
  })
  .on('idle', () => {
    console.log('emit idle')
  })
  .on('resume', () => {
    console.log('emit resume')
  })

Thread.create(() => {
  process
    .on('suspend', () => {
      console.log('emit suspend thread')
    })
    .on('resume', () => {
      console.log('emit resume thread')
    })
    .suspend()

  setTimeout(() => {}, 100) // Keep the thread alive
})

setTimeout(() => {}, 100) // Keep the process alive
