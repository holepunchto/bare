const test = require('brittle')

test('suspend + resume', async (t) => {
  t.plan(2)

  Bare.on('suspend', () => t.pass('suspended'))
    .on('idle', () => t.fail('should not idle'))
    .on('resume', () => t.pass('resumed'))
    .on('wakeup', () => t.fail('should not wake up'))

  Bare.suspend()
  Bare.resume()

  t.teardown(() => resetListeners())
})

test('suspend + resume with linger', function (t) {
  t.plan(2)

  Bare.on('suspend', (linger) => t.is(linger, 1000, 'suspended with linger'))
    .on('idle', () => t.fail('should not idle'))
    .on('resume', () => t.pass('resumed'))
    .on('wakeup', () => t.fail('should not wake up'))

  Bare.suspend(1000)
  Bare.resume()

  t.teardown(() => resetListeners())
})

test('suspend + resume on suspend', function (t) {
  t.plan(2)

  Bare.on('suspend', () => {
    t.pass('suspended')
    Bare.resume()
  })
    .on('idle', () => t.fail('should not idle'))
    .on('resume', () => t.pass('resumed'))
    .on('wakeup', () => t.fail('should not wake up'))
    .suspend()

  t.teardown(() => resetListeners())
})

test('suspend + resume on idle', function (t) {
  t.plan(3)

  Bare.on('suspend', () => t.pass('suspended'))
    .on('idle', () => {
      t.pass('idled')
      Bare.resume()
    })
    .on('resume', () => t.pass('resumed'))
    .on('wakeup', () => t.fail('should not wake up'))
    .suspend()

  t.teardown(() => resetListeners())
})

test('suspend + resume + suspend', function (t) {
  t.plan(5)

  const [suspend, resume, idle] = [1, 2, 3]
  const expectedEvents = [suspend, resume, suspend, idle, resume]

  Bare.on('suspend', () => t.is(expectedEvents.shift(), suspend, 'suspended'))
    .on('idle', () => {
      t.is(expectedEvents.shift(), idle, 'idled')
      Bare.resume()
    })
    .on('resume', () => t.is(expectedEvents.shift(), resume, 'resumed'))
    .on('wakeup', () => t.fail('should not wake up'))

  Bare.suspend()
  Bare.resume()
  Bare.suspend()

  t.teardown(() => resetListeners())
})

test('suspend + resume + suspend with linger', function (t) {
  t.plan(7)

  const [suspend, resume, idle] = [1, 2, 3]
  const expectedEvents = [suspend, resume, suspend, idle, resume]

  Bare.on('suspend', (linger) => {
    t.is(expectedEvents.shift(), suspend, 'suspended')
    t.is(linger, 2000, 'linger value')
  })
    .on('idle', () => {
      t.is(expectedEvents.shift(), idle, 'idled')
      Bare.resume()
    })
    .on('resume', () => t.is(expectedEvents.shift(), resume, 'resumed'))
    .on('wakeup', () => t.fail('should not wake up'))

  Bare.suspend(1000)
  Bare.resume()
  Bare.suspend(2000)

  t.teardown(() => resetListeners())
})

test('suspend + resume + suspend on suspend', function (t) {
  t.plan(5)

  const [suspend, resume, idle] = [1, 2, 3]
  const expectedEvents = [suspend, resume, suspend, idle, resume]

  Bare.on('suspend', () => {
    const isFirstRun = expectedEvents.length === 5

    t.is(expectedEvents.shift(), suspend, 'suspended')

    if (isFirstRun) {
      Bare.resume()
      Bare.suspend()
    }
  })
    .on('idle', () => {
      t.is(expectedEvents.shift(), idle, 'idled')
      Bare.resume()
    })
    .on('resume', () => t.is(expectedEvents.shift(), resume, 'resumed'))
    .on('wakeup', () => t.fail('should not wake up'))
    .suspend()

  t.teardown(() => resetListeners())
})

function resetListeners() {
  Bare.removeAllListeners('suspend')
    .removeAllListeners('idle')
    .removeAllListeners('resume')
    .removeAllListeners('wakeup')
}
