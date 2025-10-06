const assert = require('bare-assert')

let suspended = false
let idled = false
let awake = 0

Bare.on('exit', () => {
  assert(suspended, 'Should have suspended')
  assert(idled, 'Should have idled')
  assert(awake === 2, 'Should have woken up 2 times')
})
  .on('suspend', () => {
    console.log('emit suspend')
    suspended = true
  })
  .on('idle', () => {
    console.log('emit idle')
    if (idled) Bare.resume()
    else {
      idled = true
      Bare.wakeup(100)
    }
  })
  .on('resume', () => {
    console.log('emit resume')
    assert(suspended)
  })
  .on('wakeup', (deadline) => {
    console.log('emit wakeup')
    if (awake++) return
    Bare.wakeup(100)
  })

Bare.suspend()
