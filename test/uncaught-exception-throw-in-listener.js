// An exception thrown by an `uncaughtException` listener cannot be reported as
// another uncaught exception without recursing, so it's reported in place of the
// original exception and terminates the process.

const t = require('bare-tap')
const abort = require('./helpers/abort')

t.plan(2)

const { aborted, stderr } = abort('uncaught-exception-throw-in-listener.js')

t.ok(aborted)
t.ok(stderr.includes('Uncaught Error: from listener'))
