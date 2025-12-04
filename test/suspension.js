const test = require('brittle')
const { Thread } = Bare

test('suspend + resume', (t) => {
  t.plan(2)

  Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

  t.teardown(() =>
    Bare.off('suspend', onsuspend)
      .off('idle', onidle)
      .off('resume', onresume)
      .off('wakeup', onwakeup)
  )

  Bare.suspend()
  Bare.resume()

  function onsuspend() {
    t.pass('suspended')
  }

  function onidle() {
    t.fail('should not idle')
  }

  function onresume() {
    t.pass('resumed')
  }

  function onwakeup() {
    t.fail('should not wake up')
  }
})

test('suspend + resume with linger', (t) => {
  t.plan(2)

  Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

  t.teardown(() =>
    Bare.off('suspend', onsuspend)
      .off('idle', onidle)
      .off('resume', onresume)
      .off('wakeup', onwakeup)
  )

  Bare.suspend(1000)
  Bare.resume()

  function onsuspend(linger) {
    t.is(linger, 1000, 'suspended with linger')
  }

  function onidle() {
    t.fail('should not idle')
  }

  function onresume() {
    t.pass('resumed')
  }

  function onwakeup() {
    t.fail('should not wake up')
  }
})

test('suspend + resume with thread', (t) => {
  const thread = new Thread(() => {
    Bare.on('suspend', () => console.log('suspended'))
      .on('idle', () => {
        console.log('idled')
        Bare.resume()
      })
      .on('resume', () => console.log('resumed'))
  })

  thread.suspend()
  thread.join()
})

test('suspend + resume on suspend', (t) => {
  t.plan(2)

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
  }

  function onwakeup() {
    t.fail('should not wake up')
  }
})

test('suspend + resume on idle', (t) => {
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
  }

  function onwakeup() {
    t.fail('should not wake up')
  }
})

test('suspend + resume + suspend', (t) => {
  t.plan(5)

  Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

  t.teardown(() =>
    Bare.off('suspend', onsuspend)
      .off('idle', onidle)
      .off('resume', onresume)
      .off('wakeup', onwakeup)
  )

  Bare.suspend()
  Bare.resume()
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
  }

  function onwakeup() {
    t.fail('should not wake up')
  }
})

test('suspend + resume + suspend with linger', (t) => {
  t.plan(5)

  Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

  t.teardown(() =>
    Bare.off('suspend', onsuspend)
      .off('idle', onidle)
      .off('resume', onresume)
      .off('wakeup', onwakeup)
  )

  Bare.suspend(1000)
  Bare.resume()
  Bare.suspend(2000)

  function onsuspend(linger) {
    t.is(linger, 2000)
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

test('suspend + resume + suspend on suspend', (t) => {
  t.plan(5)

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
})

test('suspend on idle', (t) => {
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
    Bare.suspend()
    Bare.resume()
  }

  function onresume() {
    t.pass('resumed')
  }

  function onwakeup() {
    t.fail('should not wake up')
  }
})

test('suspend + suspend + resume on thread', async (t) => {
  const thread = new Thread(() => {
    const assert = require('bare-assert')

    Bare.on('suspend', () => console.log('suspended'))
      .on('idle', () => console.log('idled'))
      .on('resume', () => Bare.off('exit', onexit))
      .on('exit', onexit)

    function onexit() {
      assert(false, 'should not exit')
    }
  })

  thread.suspend()
  await sleep(100)

  thread.suspend()
  await sleep(100)

  thread.resume()
  thread.join()
})

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms))
}
