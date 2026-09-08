// An exception raised from a microtask enqueued by an `uncaughtException`
// listener is reported once the report that enqueued it has finished, rather
// than nesting within it.

const t = require('bare-tap')

t.plan(2)

let reports = 0

Bare.on('uncaughtException', (err) => {
  if (++reports === 1) {
    t.equal(err.message, 'boom')

    queueMicrotask(() => {
      throw new Error('from microtask')
    })
  } else {
    t.equal(err.message, 'from microtask')
  }
})

throw new Error('boom')
