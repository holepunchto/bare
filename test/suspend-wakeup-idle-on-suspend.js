const t = require('bare-tap')

t.plan(3)

Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

Bare.suspend()
Bare.wakeup(100)

function onsuspend() {
  t.pass('suspended')
  Bare.idle()
}

function onidle() {
  t.fail('should not idle')
}

function onresume() {
  t.pass('resumed')
}

function onwakeup(deadline) {
  t.equal(deadline, 100, 'woke up')
  Bare.resume()
}
