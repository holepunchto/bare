// An `unhandledRejection` listener handles the rejection; the process keeps
// running.

const t = require('bare-tap')

t.plan(2)

Bare.on('unhandledRejection', (reason) => {
  t.equal(reason.message, 'boom')
})

setTimeout(() => {
  t.pass()
}, 0)

Promise.reject(new Error('boom'))
