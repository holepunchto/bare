const test = require('brittle')

test('basic', function (t) {
  t.plan(2)

  Bare.on('suspend', () => {
    t.pass('suspended')
    Bare.wakeup(100)
  })
    .on('idle', () => t.fail('should not idle'))
    .on('resume', () => t.fail('should not resume'))
    .on('wakeup', (deadline) => t.is(deadline, 100, 'woke up'))
    .suspend()
})
