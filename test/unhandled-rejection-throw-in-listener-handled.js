// An exception thrown by an `unhandledRejection` listener is reported as an
// uncaught exception, which an `uncaughtException` listener may handle.

const t = require('bare-tap')

t.plan(2)

Bare.on('unhandledRejection', (reason) => {
  t.equal(reason.message, 'boom')

  throw new Error('from listener')
})

Bare.on('uncaughtException', (err) => {
  t.equal(err.message, 'from listener')
})

Promise.reject(new Error('boom'))
