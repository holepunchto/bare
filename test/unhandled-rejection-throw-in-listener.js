// An exception thrown by an `unhandledRejection` listener is reported as an
// uncaught exception, even though the exception is still pending when the report
// begins.

const t = require('bare-tap')
const abort = require('./helpers/abort')

t.plan(2)

const { aborted, stderr } = abort('unhandled-rejection-throw-in-listener.js')

t.ok(aborted)
t.ok(stderr.includes('Uncaught Error: from listener'))
