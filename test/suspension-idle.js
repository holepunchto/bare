const test = require('brittle')

test('idle on suspend', (t) => {
  t.plan(3)

  Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

  t.teardown(() =>
    Bare.off('suspend', onsuspend)
      .off('idle', onidle)
      .off('resume', onresume)
      .off('wakeup', onwakeup)
  )

  let timer

  Bare.suspend()

  function onsuspend() {
    t.pass('suspended')
    timer = setTimeout(() => t.fail('should not execute the timer callback'), 10000)
    Bare.idle()
  }

  function onidle() {
    t.pass('idled')
    Bare.resume()
  }

  function onresume() {
    t.pass('resumed')
    clearTimeout(timer)
  }

  function onwakeup() {
    t.fail('should not wake up')
  }
})

test('suspend + idle', (t) => {
  t.plan(3)

  Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

  t.teardown(() =>
    Bare.off('suspend', onsuspend)
      .off('idle', onidle)
      .off('resume', onresume)
      .off('wakeup', onwakeup)
  )

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
})

test('idle on idle', (t) => {
  t.plan(3)

  Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

  t.teardown(() =>
    Bare.off('suspend', onsuspend)
      .off('idle', onidle)
      .off('resume', onresume)
      .off('wakeup', onwakeup)
  )

  let timer

  Bare.suspend()

  function onsuspend() {
    t.pass('suspended')
    timer = setTimeout(() => t.fail('should not execute the timer callback'), 10000)
    Bare.idle()
  }

  function onidle() {
    t.pass('idled')
    Bare.idle()
    Bare.resume()
  }

  function onresume() {
    t.pass('resumed')
    clearTimeout(timer)
  }

  function onwakeup() {
    t.fail('should not wake up')
  }
})

test('idle on resume', (t) => {
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
  }

  function onidle() {
    t.pass('idled')
    Bare.resume()
  }

  function onresume() {
    t.pass('resumed')
    Bare.idle()
  }

  function onwakeup() {
    t.fail('should not wake up')
  }
})

test('idle before suspend', (t) => {
  Bare.on('idle', onidle)

  t.teardown(() => Bare.off('idle', onidle))

  Bare.idle()

  function onidle() {
    t.fail('should not idle')
  }
})
