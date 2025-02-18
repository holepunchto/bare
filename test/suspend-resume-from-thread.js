const { Thread } = Bare

Bare.on('suspend', () => {
  console.log('emit suspend')
})
  .on('idle', () => {
    console.log('emit idle')
  })
  .on('resume', () => {
    console.log('emit resume')
  })
  .suspend()

Thread.create(() => {
  Bare.on('suspend', () => {
    console.log('emit suspend thread')
  })
    .on('idle', () => {
      console.log('emit idle thread')
      Bare.resume()
    })
    .on('resume', () => {
      console.log('emit resume thread')
    })

  setTimeout(() => {}, 100) // Keep the thread alive
})
