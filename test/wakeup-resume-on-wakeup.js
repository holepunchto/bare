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
    idled = true
    Bare.wakeup(100)
  })
  .on('resume', () => {
    console.log('emit resume')
    assert(suspended)
  })
  .on('wakeup', () => {
    console.log('emit wakeup')
    awake = true
    Bare.resume()
  })

Bare.suspend()
