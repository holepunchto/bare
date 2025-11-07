const test = require('brittle')

test('basic', function (t) {
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
})
