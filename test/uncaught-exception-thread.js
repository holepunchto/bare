// Uncaught exceptions are reported to the thread they were raised on. If the
// listener isn't consulted the thread aborts, taking the process with it.

const t = require('bare-tap')
const { Thread } = Bare

t.plan(1)

const thread = new Thread(() => {
  Bare.on('uncaughtException', (err) => {
    if (err.message !== 'boom') throw err
  })

  throw new Error('boom')
})

thread.join()

t.pass()
