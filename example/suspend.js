process
  .on('suspend', () => {
    console.log('process suspended')
  })
  .on('idle', () => {
    console.log('process is idle')

    process.resume()
  })
  .on('resume', () => {
    console.log('process resumed')
  })
  .suspend()
