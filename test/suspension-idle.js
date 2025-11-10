const test = require('brittle')

test('idle on suspend', function (t) {
  t.plan(3)

  let timer

  Bare.on('suspend', () => {
    t.pass('suspended')
    timer = setTimeout(() => t.fail('should not execute the timer callback'), 10000)
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

  t.teardown(() => resetListeners())
})

test('idle on idle', function (t) {
  t.plan(3)

  Bare.on('suspend', () => t.pass('suspended'))
    .on('idle', () => {
      t.pass('idled')
      Bare.idle()
      Bare.resume()
    })
    .on('resume', () => t.pass('resumed'))
    .on('wakeup', () => t.fail('should not wake up'))
    .suspend()

  t.teardown(() => resetListeners())
})

test('idle on resume', function (t) {
  t.plan(3)

  Bare.on('suspend', () => t.pass('suspended'))
    .on('idle', () => {
      t.pass('idled')
      Bare.resume()
    })
    .on('resume', () => {
      t.pass('resumed')
      Bare.idle()
    })
    .on('wakeup', () => t.fail('should not wake up'))
    .suspend()

  t.teardown(() => resetListeners())
})

function resetListeners() {
  Bare.removeAllListeners('suspend')
    .removeAllListeners('idle')
    .removeAllListeners('resume')
    .removeAllListeners('wakeup')
}
