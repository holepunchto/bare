// A microtask enqueued from an `exit` listener must run; the microtask queue
// is flushed before the runtime tears down.

Bare.exitCode = 1

Bare.on('exit', () => {
  queueMicrotask(() => {
    Bare.exitCode = 0
  })
})
