const test = require('brittle')

test('basic', function (t) {
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
})
