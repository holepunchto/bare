// An exception thrown by an `unhandledRejection` listener is reported as an
// uncaught exception, even though the exception is still pending when the report
// begins.

Bare.on('unhandledRejection', () => {
  throw new Error('from listener')
})

Promise.reject(new Error('boom'))
