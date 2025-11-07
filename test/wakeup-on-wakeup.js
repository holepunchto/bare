const test = require('brittle')

test('basic', function (t) {
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
})
