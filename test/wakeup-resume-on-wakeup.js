const t = require('./harness')

t.plan(4)

Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

Bare.suspend()

function onsuspend() {
  t.pass('suspended')
}

function onidle() {
  t.pass('idled')
  Bare.wakeup(100)
}

function onresume() {
  t.pass('resumed')
}

function onwakeup(deadline) {
  t.equal(deadline, 100, 'woke up')
  Bare.resume()
}
