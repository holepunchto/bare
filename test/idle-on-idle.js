Bare.on('suspend', () => {
  console.log('emit suspend')
})
  .on('idle', () => {
    console.log('emit idle')
    Bare.idle()
    Bare.resume()
  })
  .on('resume', () => {
    console.log('emit resume')
  })
  .suspend()
