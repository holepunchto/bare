const test = require('brittle')

test('basic', function (t) {
  t.plan(3)

  Bare.on('suspend', () => t.pass('suspended'))
    .on('idle', () => {
      t.pass('idled')
      Bare.resume()
    })
    .on('resume', () => t.pass('resumed'))
    .on('wakeup', () => t.fail('should not wake up'))
    .suspend()
})
