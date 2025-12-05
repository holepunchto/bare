const t = require('bare-tap')

t.plan(2)

Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

Bare.suspend(1000)
Bare.resume()

function onsuspend(linger) {
  t.equal(linger, 1000, 'suspended with linger')
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
