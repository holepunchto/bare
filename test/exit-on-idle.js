Bare.on('idle', () => {
  console.log('emit idle')
  Bare.exit()
}).suspend()
