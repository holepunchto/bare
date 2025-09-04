const assert = require('bare-assert')

let suspended = false
let idled = false
let awake = false

Bare.on('exit', () => {
  assert(suspended, 'Should have suspended')
  assert(idled, 'Should have idled')
  assert(awake, 'Should have woken up')
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
      Bare.wakeup()
    }
  })
  .on('resume', () => {
    console.log('emit resume')
    assert(suspended)
  })
  .on('wakeup', (deadline) => {
    console.log('emit wakeup')
    assert(!awake)
    awake = true
    Bare.wakeup()
  })

Bare.suspend()
