const t = require('./harness')

t.plan(4)

Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

let suspended = false

Bare.suspend()

function onsuspend() {
  if (suspended++) t.fail('should not suspend twice')
  else t.pass('suspended')
}

function onidle() {
  t.pass('idled')
  Bare.wakeup(100)
}

function onresume() {
  t.pass('resumed')
}

function onwakeup() {
  t.pass('woke up')
  Bare.suspend()
  Bare.resume()
}
