const test = require('brittle')

test('basic', async (t) => {
  t.plan(4)

  Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

  t.teardown(() =>
    Bare.off('suspend', onsuspend)
      .off('idle', onidle)
      .off('resume', onresume)
      .off('wakeup', onwakeup)
  )

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
    t.is(deadline, 100, 'woke up')
    Bare.resume()
  }
})

test('wakeup on suspend', (t) => {
  t.plan(3)

  Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

  t.teardown(() =>
    Bare.off('suspend', onsuspend)
      .off('idle', onidle)
      .off('resume', onresume)
      .off('wakeup', onwakeup)
  )

  Bare.suspend()

  function onsuspend() {
    t.pass('suspended')
    Bare.wakeup(100)
  }

  function onidle() {
    t.fail('should not idle')
  }

  function onresume() {
    t.pass('resumed')
  }

  function onwakeup(deadline) {
    t.is(deadline, 100, 'woke up')
    Bare.resume()
  }
})

test('wakeup on resume', (t) => {
  t.plan(3)

  Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

  t.teardown(() =>
    Bare.off('suspend', onsuspend)
      .off('idle', onidle)
      .off('resume', onresume)
      .off('wakeup', onwakeup)
  )

  Bare.suspend()

  function onsuspend() {
    t.pass('suspended')
    Bare.resume()
  }

  function onidle() {
    t.fail('should not idle')
  }

  function onresume() {
    t.pass('resumed')
    Bare.wakeup(100)
    setTimeout(() => t.pass('flushed'), 10) // Let the tick flush
  }

  function onwakeup() {
    t.fail('should not wake up')
  }
})

test('wakeup on wakeup', (t) => {
  t.plan(6)

  Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

  t.teardown(() =>
    Bare.off('suspend', onsuspend)
      .off('idle', onidle)
      .off('resume', onresume)
      .off('wakeup', onwakeup)
  )

  let idled = false
  let awake = false

  Bare.suspend()

  function onsuspend() {
    t.pass('suspended')
  }

  function onidle() {
    t.pass('idled')
    if (idled++) Bare.resume()
    else Bare.wakeup(100)
  }

  function onresume() {
    t.pass('resumed')
  }

  function onwakeup() {
    t.pass('woke up')
    if (awake++) Bare.resume()
    else {
      Bare.wakeup(100)
      setTimeout(() => t.pass('flushed'), 10) // Let the tick flush
    }
  }
})

test('wakeup + suspend on wakeup', (t) => {
  t.plan(4)

  Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

  t.teardown(() =>
    Bare.off('suspend', onsuspend)
      .off('idle', onidle)
      .off('resume', onresume)
      .off('wakeup', onwakeup)
  )

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

  function onwakeup(deadline) {
    t.is(deadline, 100, 'woke up')
    Bare.suspend()
    setTimeout(() => Bare.resume(), 10) // Let the tick flush
  }
})

test('wakeup + resume', (t) => {
  t.plan(4)

  Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

  t.teardown(() =>
    Bare.off('suspend', onsuspend)
      .off('idle', onidle)
      .off('resume', onresume)
      .off('wakeup', onwakeup)
  )

  Bare.suspend()

  function onsuspend() {
    t.pass('suspended')
  }

  function onidle() {
    t.pass('idled')
    Bare.wakeup(100)
    Bare.resume()
  }

  function onresume() {
    t.pass('resumed')
  }

  function onwakeup(deadline) {
    t.is(deadline, 100, 'woke up')
  }
})

test('wakeup + resume on wakeup', (t) => {
  t.plan(4)

  Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

  t.teardown(() =>
    Bare.off('suspend', onsuspend)
      .off('idle', onidle)
      .off('resume', onresume)
      .off('wakeup', onwakeup)
  )

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
    t.is(deadline, 100, 'woke up')
    Bare.resume()
  }
})

test('wakeup exceed deadline', (t) => {
  t.plan(5)

  Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

  t.teardown(() =>
    Bare.off('suspend', onsuspend)
      .off('idle', onidle)
      .off('resume', onresume)
      .off('wakeup', onwakeup)
  )

  let idled = false
  let finished = false

  Bare.suspend()

  function onsuspend() {
    t.pass('suspended')
  }

  function onidle() {
    t.pass('idled')
    if (idled++) {
      t.absent(finished, 'callback should not have run')
      Bare.resume()
    } else {
      Bare.wakeup(100)
    }
  }

  function onresume() {
    t.pass('resumed')
  }

  function onwakeup(deadline) {
    setTimeout(() => {
      finished = true
    }, deadline + 50)
  }
})

test('wakeup before suspend', (t) => {
  Bare.on('wakeup', () => t.fail('should not wake up'))

  Bare.wakeup(100)
})
