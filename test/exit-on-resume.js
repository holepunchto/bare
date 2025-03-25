Bare.on('suspend', () => {
  Bare.resume()
})
  .on('resume', () => {
    Bare.exit()
  })
  .suspend()
