process
  .on('suspend', () => {
    console.log('process suspended')

    const timer = setTimeout(() => process.resume(), 1000)
    timer.unref()
  })
  .on('resume', () => {
    console.log('process resumed')
  })
  .suspend()
