const test = require('brittle')

test('basic', function (t) {
  t.plan(3)

  let timer

  Bare.on('suspend', () => {
    t.pass('suspended')
    timer = setTimeout(() => assert(false), 10000)
    Bare.idle()
  })
    .on('idle', () => {
      t.pass('idled')
      Bare.resume()
    })
    .on('resume', () => {
      t.pass('resumed')
      clearTimeout(timer)
    })
    .on('wakeup', () => t.fail('should not wake up'))
    .suspend()
})
