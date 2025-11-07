const test = require('brittle')

test('basic', function (t) {
  t.plan(3)

  Bare.on('suspend', () => t.pass('suspended'))
    .on('idle', () => {
      t.pass('idled')
      Bare.wakeup(100)
    })
    .on('resume', () => t.fail('should not resume'))
    .on('wakeup', (deadline) => t.is(deadline, 100, 'woke up'))
    .suspend()
})
