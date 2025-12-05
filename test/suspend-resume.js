const t = require('bare-tap')

t.plan(2)

Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

Bare.suspend()
Bare.resume()

function onsuspend() {
  t.pass('suspended')
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
