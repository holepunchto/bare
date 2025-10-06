const assert = require('bare-assert')

let suspended = false

Bare.on('exit', () => {
  assert(suspended, 'Should have suspended')
})
  .on('suspend', () => {
    console.log('emit suspend')
    suspended = true
    Bare.wakeup(100)
  })
  .on('idle', () => {
    console.log('emit idle')
    Bare.resume()
  })
  .on('resume', () => {
    console.log('emit resume')
    assert(suspended)
  })
  .on('wakeup', () => {
    console.log('emit wakeup')
    awake = true
  })

Bare.suspend()
