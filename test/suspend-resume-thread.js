const { Thread } = Bare

const thread = new Thread(() => {
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
})

thread.suspend()
thread.join()
