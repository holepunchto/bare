const t = require('bare-tap')

t.plan(3)

Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

Bare.suspend()
Bare.resume()

function onsuspend() {
  t.pass('suspended')
  setTimeout(() => t.pass('ran pending work'), 10)
  Bare.idle()
}

function onidle() {
  t.fail('should not idle')
}

function onresume() {
  t.pass('resumed')
}

function onwakeup() {
  t.fail('should not wake up')
}
