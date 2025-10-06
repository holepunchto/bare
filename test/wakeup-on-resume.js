const assert = require('bare-assert')

let suspended = false

Bare.on('exit', () => {
  assert(suspended, 'Should have suspended')
})
  .on('suspend', () => {
    console.log('emit suspend')
    suspended = true
  })
  .on('idle', () => {
    console.log('emit idle')
    Bare.resume()
  })
  .on('resume', () => {
    console.log('emit resume')
    assert(suspended)
    Bare.wakeup(100)
  })
  .on('wakeup', (deadline) => {
    assert(false, 'Should not wake up')
  })

Bare.suspend()
