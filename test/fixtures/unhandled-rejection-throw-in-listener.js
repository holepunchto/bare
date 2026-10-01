Bare.on('unhandledRejection', () => {
  throw new Error('from listener')
})

Promise.reject(new Error('boom'))
