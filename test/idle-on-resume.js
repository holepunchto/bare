Bare.on('suspend', () => {
  console.log('emit suspend')
})
  .on('idle', () => {
    console.log('emit idle')
    Bare.resume()
  })
  .on('resume', () => {
    console.log('emit resume')
    Bare.idle()
  })
  .suspend()
