const t = require('bare-tap')

t.plan(2)

Bare.on('suspend', onsuspend).on('idle', onidle)

Bare.suspend(2 ** 32 + 1000)

function onsuspend(linger) {
  t.equal(linger, 0x7fffffff, 'linger is clamped')
}

function onidle() {
  t.pass('idled')
  Bare.resume()
}
