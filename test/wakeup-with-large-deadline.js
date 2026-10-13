const t = require('bare-tap')

t.plan(5)

Bare.on('idle', onidle).on('wakeup', onwakeup)

t.throws(() => Bare.wakeup(0x80000000), RangeError, 'throws just above the limit')
t.throws(() => Bare.wakeup(3_000_000_000), RangeError, 'throws instead of wrapping')
t.throws(() => Bare.wakeup(Infinity), RangeError, 'throws for infinity')

let woke = false

Bare.suspend()

function onidle() {
  if (woke) return

  t.pass('idled')
  Bare.wakeup(0x7fffffff)
}

function onwakeup(deadline) {
  woke = true
  t.equal(deadline, 0x7fffffff, 'accepts the limit')
  Bare.resume()
}
