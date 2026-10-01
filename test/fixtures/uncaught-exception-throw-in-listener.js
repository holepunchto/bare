Bare.on('uncaughtException', () => {
  throw new Error('from listener')
})

throw new Error('boom')
