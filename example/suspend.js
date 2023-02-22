process
  .on('suspend', () => {
    console.log('process suspended')

    process.resume()
  })
  .on('resume', () => {
    console.log('process resumed')
  })
  .suspend()
