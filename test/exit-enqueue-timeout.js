// A timer enqueued from an `exit` listener must never run; the runtime is
// already tearing down and gives the loop no further iteration to run it on.

Bare.exitCode = 1

Bare.on('exit', () => {
  Bare.exitCode = 0

  setTimeout(() => {
    Bare.exitCode = 1
  }, 0)
})
