process.on('suspend', () => {
  const timer = setTimeout(() => process.resume(), 1000)
  timer.unref()
})

process.suspend()
