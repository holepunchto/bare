const t = require('bare-tap')

t.plan(3)

Bare.on('idle', onidle).on('wakeup', onwakeup)

let woke = false

Bare.suspend()

function onidle() {
  if (woke) return

  t.pass('idled')
  Bare.wakeup(3_000_000_000)
}

function onwakeup(deadline) {
  woke = true
  t.equal(deadline, 0x7fffffff, 'deadline is clamped')
  t.pass('woke up')
  Bare.resume()
}
