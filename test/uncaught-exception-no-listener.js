// An uncaught exception with no `uncaughtException` listener is reported and
// terminates the process.

const t = require('bare-tap')
const abort = require('./helpers/abort')

t.plan(2)

const { aborted, stderr } = abort('uncaught-exception-no-listener.js')

t.ok(aborted)
t.ok(stderr.includes('Uncaught Error: boom'))
