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
    assert(false, 'Should not idle')
  })
  .on('resume', () => {
    console.log('emit resume')
    assert(suspended)
  })

Bare.suspend()
Bare.resume()
