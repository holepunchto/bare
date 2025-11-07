const test = require('brittle')

test('basic', function (t) {
  t.plan(7)

  const [suspend, resume, idle] = [1, 2, 3]
  const expectedEvents = [suspend, resume, suspend, idle, resume]

  Bare.on('suspend', (linger) => {
    t.is(expectedEvents.shift(), suspend, 'suspended')
    t.is(linger, 2000)
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
})
