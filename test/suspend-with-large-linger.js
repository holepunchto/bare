const t = require('bare-tap')

t.plan(5)

Bare.on('suspend', onsuspend).on('idle', onidle)

t.throws(() => Bare.suspend(0x80000000), RangeError, 'throws just above the limit')
t.throws(() => Bare.suspend(2 ** 32 + 1000), RangeError, 'throws instead of wrapping')
t.throws(() => Bare.suspend(Infinity), RangeError, 'throws for infinity')

Bare.suspend(0x7fffffff)

function onsuspend(linger) {
  t.equal(linger, 0x7fffffff, 'accepts the limit')
}

function onidle() {
  t.pass('idled')
  Bare.resume()
}
