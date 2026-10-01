// An unhandled rejection with no `unhandledRejection` listener is reported and
// terminates the process.

const t = require('bare-tap')
const abort = require('./helpers/abort')

t.plan(2)

const { aborted, stderr } = abort('unhandled-rejection-no-listener.js')

t.ok(aborted)
t.ok(stderr.includes('Uncaught (in promise) Error: boom'))
