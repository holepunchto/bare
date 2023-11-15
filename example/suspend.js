/* global Bare */
Bare
  .on('suspend', () => {
    console.log('process suspended')
  })
  .on('idle', () => {
    console.log('process is idle')

    Bare.resume()
  })
  .on('resume', () => {
    console.log('process resumed')
  })
  .suspend()
