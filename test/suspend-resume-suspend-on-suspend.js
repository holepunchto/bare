const t = require('./harness')

t.plan(5)

Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

let suspended = false

Bare.suspend()

function onsuspend() {
  t.pass('suspended')
  if (suspended++) return
  Bare.resume()
  Bare.suspend()
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
