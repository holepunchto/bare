const t = require('./harness')

t.plan(3)

Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

Bare.suspend()

function onsuspend() {
  t.pass('suspended')
}

function onidle() {
  Bare.suspend()
  Bare.resume()
  t.pass('idled')
}

function onresume() {
  t.pass('resumed')
}

function onwakeup() {
  t.fail('should not wake up')
}
