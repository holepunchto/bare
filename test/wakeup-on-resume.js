const t = require('bare-tap')

t.plan(3)

Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

Bare.suspend()

function onsuspend() {
  t.pass('suspended')
  Bare.resume()
}

function onidle() {
  t.fail('should not idle')
}

function onresume() {
  t.pass('resumed')
  Bare.wakeup(100)
  setTimeout(() => t.pass('flushed'), 10) // Let the tick flush
}

function onwakeup() {
  t.fail('should not wake up')
}
