// An exception thrown by an `uncaughtException` listener cannot be reported as
// another uncaught exception without recursing, so it's reported in place of the
// original exception and terminates the process.

Bare.on('uncaughtException', () => {
  throw new Error('from listener')
})

throw new Error('boom')
