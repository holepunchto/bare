// An `uncaughtException` listener handles the exception; the process keeps
// running.

const t = require('bare-tap')

t.plan(2)

Bare.on('uncaughtException', (err) => {
  t.equal(err.message, 'boom')
})

setTimeout(() => {
  t.pass()
}, 0)

throw new Error('boom')
