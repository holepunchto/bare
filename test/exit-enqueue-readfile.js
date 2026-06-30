// File I/O enqueued from an `exit` listener must never run; the runtime is
// already tearing down and the environment is destroyed before the loop runs
// again, so the threadpool request can never complete back into JavaScript.

const fs = require('bare-fs')

Bare.exitCode = 1

Bare.on('exit', () => {
  Bare.exitCode = 0

  fs.readFile(__filename, () => {
    Bare.exitCode = 1
  })
})
