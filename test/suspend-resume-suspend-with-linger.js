const t = require('bare-tap')

t.plan(5)

Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

Bare.suspend(1000)
Bare.resume()
Bare.suspend(2000)

function onsuspend(linger) {
  t.equal(linger, 2000)
}

function onidle() {
  t.pass('idled')
  Bare.resume()
}

function onresume() {
  t.pass('resumed')
}

function onwakeup() {
  t.fail('should not wake up')
}
