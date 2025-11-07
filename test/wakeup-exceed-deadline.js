const test = require('brittle')

test('basic', function (t) {
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
})
