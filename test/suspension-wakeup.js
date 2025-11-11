const test = require('brittle')

test('basic', async (t) => {
  t.plan(3)

  Bare.on('suspend', () => t.pass('suspended'))
    .on('idle', () => {
      t.pass('idled')
      Bare.wakeup(100)
    })
    .on('resume', () => t.fail('should not resume'))
    .on('wakeup', (deadline) => t.is(deadline, 100, 'woke up'))
    .suspend()

  t.teardown(async () => {
    resetListeners()
    await forceRelease()
  })
})

test('wakeup on suspend', (t) => {
  t.plan(2)

  Bare.on('suspend', () => {
    t.pass('suspended')
    Bare.wakeup(100)
  })
    .on('idle', () => t.fail('should not idle'))
    .on('resume', () => t.fail('should not resume'))
    .on('wakeup', (deadline) => t.is(deadline, 100, 'woke up'))
    .suspend()

  t.teardown(async () => {
    resetListeners()
    await forceRelease()
  })
})

test('wakeup on release', (t) => {
  t.plan(2)

  Bare.on('suspend', () => {
    t.pass('suspended')
    Bare.resume()
  })
    .on('idle', () => t.fail('should not idle'))
    .on('resume', () => {
      t.pass('resumed')
      Bare.wakeup(100)
    })
    .on('wakeup', (deadline) => t.fail('should not wake up'))
    .suspend()

  t.teardown(async () => {
    resetListeners()
    await forceRelease()
  })
})

test('wakeup on wakeup', (t) => {
  t.plan(4)

  const [suspend, idle, wakeup] = [1, 2, 3]
  const expectedEvents = [suspend, idle, wakeup, wakeup]

  let isFirstRun = true

  Bare.on('suspend', () => t.is(expectedEvents.shift(), suspend, 'suspended'))
    .on('idle', () => {
      t.is(expectedEvents.shift(), idle, 'idled')
      Bare.wakeup(100)
    })
    .on('resume', () => t.fail('should not resume'))
    .on('wakeup', () => {
      t.is(expectedEvents.shift(), wakeup, 'woke up')

      if (isFirstRun) {
        isFirstRun = false
        Bare.wakeup(100)
      }
    })
    .suspend()

  t.teardown(async () => {
    resetListeners()
    await forceRelease()
  })
})

test('wakeup + suspend on wakeup', (t) => {
  t.plan(3)

  let suspended

  Bare.on('suspend', () => {
    if (suspended) t.fail('should not suspend twice')
    else {
      t.pass('suspended')
      suspended = true
    }
  })
    .on('idle', () => {
      t.pass('idled')
      Bare.wakeup(100)
    })
    .on('resume', () => t.fail('should not resume'))
    .on('wakeup', (deadline) => {
      t.is(deadline, 100, 'woke up')
      Bare.suspend()
    })
    .suspend()

  t.teardown(async () => {
    resetListeners()
    await forceRelease()
  })
})

test('wakeup + resume', (t) => {
  t.plan(4)

  Bare.on('suspend', () => t.pass('suspended'))
    .on('idle', () => {
      t.pass('idled')
      Bare.wakeup(100)
      Bare.resume()
    })
    .on('resume', () => t.pass('resumed'))
    .on('wakeup', (deadline) => t.is(deadline, 100, 'woke up'))
    .suspend()

  t.teardown(() => resetListeners())
})

test('wakeup + resume on wakeup', (t) => {
  t.plan(4)

  Bare.on('suspend', () => t.pass('suspended'))
    .on('idle', () => {
      t.pass('idled')
      Bare.wakeup(100)
    })
    .on('resume', () => t.pass('resumed'))
    .on('wakeup', (deadline) => {
      t.is(deadline, 100, 'woke up')
      Bare.resume()
    })
    .suspend()

  t.teardown(() => resetListeners())
})

test('wakeup exceed deadline', (t) => {
  t.plan(6)

  const [suspend, resume, idle, wakeup] = [1, 2, 3, 4]
  const expectedEvents = [suspend, idle, wakeup, idle, resume]

  let finished = false

  Bare.on('suspend', () => t.is(expectedEvents.shift(), suspend, 'suspended'))
    .on('idle', () => {
      const isFirstRun = expectedEvents.length === 4

      t.is(expectedEvents.shift(), idle, 'idled')

      if (isFirstRun) {
        Bare.wakeup(100)
      } else {
        t.absent(finished, 'callback should not have run')
        Bare.resume()
      }
    })
    .on('resume', () => t.is(expectedEvents.shift(), resume, 'resumed'))
    .on('wakeup', (deadline) => {
      t.is(expectedEvents.shift(), wakeup, 'woke up')
      setTimeout(() => (finished = true), deadline + 50)
    })
    .suspend()

  t.teardown(() => resetListeners())
})

test('wakeup before suspend', (t) => {
  Bare.on('wakeup', () => t.fail('should not wake up'))

  Bare.wakeup(100)

  t.teardown(() => Bare.removeAllListeners('wakeup'))
})

function resetListeners() {
  Bare.removeAllListeners('suspend')
    .removeAllListeners('idle')
    .removeAllListeners('resume')
    .removeAllListeners('wakeup')
}

async function forceRelease() {
  await new Promise((resolve) => setTimeout(resolve))
  Bare.resume()
  await new Promise((resolve) => setTimeout(resolve))
}
