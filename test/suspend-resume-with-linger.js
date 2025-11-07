const test = require('brittle')

test('basic', function (t) {
  t.plan(2)

  Bare.on('suspend', (linger) => t.is(linger, 1000, 'suspended with linger'))
    .on('idle', () => t.fail('should not idle'))
    .on('resume', () => t.pass('resumed'))
    .on('wakeup', () => t.fail('should not wake up'))

  Bare.suspend(1000)
  Bare.resume()
})
