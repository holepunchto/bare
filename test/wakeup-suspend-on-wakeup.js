const test = require('brittle')

test('basic', function (t) {
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
})
