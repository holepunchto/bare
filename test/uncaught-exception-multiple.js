// Uncaught exceptions that are reported one after the other are all handed to
// the listeners; only reports that nest within one another are fatal.

const t = require('bare-tap')

t.plan(2)

Bare.on('uncaughtException', (err) => {
  t.pass(err.message)
})

setTimeout(() => {
  throw new Error('first')
}, 0)

setTimeout(() => {
  throw new Error('second')
}, 0)
