Bare.on('suspend', () => {
  Bare.idle()
})
  .on('idle', () => {
    Bare.wakeup(100)
  })
  .on('wakeup', () => {
    Bare.exit()
  })
  .suspend()
