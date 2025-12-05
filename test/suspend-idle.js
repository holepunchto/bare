const t = require('bare-tap')

t.plan(3)

Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

Bare.suspend()
Bare.idle()

function onsuspend() {
  t.pass('suspended')
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
