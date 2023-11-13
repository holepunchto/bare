/* global Bare */
const { Thread } = Bare

Bare
  .on('suspend', () => {
    console.log('emit suspend')
    Bare.resume()
  })
  .on('idle', () => {
    console.log('emit idle')
  })
  .on('resume', () => {
    console.log('emit resume')
  })

Thread.create(() => {
  Bare
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
