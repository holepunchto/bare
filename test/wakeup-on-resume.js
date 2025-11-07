const test = require('brittle')

test('basic', function (t) {
  t.plan(2)

  Bare.on('suspend', () => {
    t.pass('suspended')
    Bare.resume()
  })
    .on('idle', () => t.fail('should not idle'))
    .on('resume', () => {
      t.pass('resumed')
      Bare.wakeup(100)
    })
    .on('wakeup', (deadline) => t.fail('should not wake up'))
    .suspend()
})
