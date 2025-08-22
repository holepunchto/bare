const assert = require('bare-assert')

let suspended = false

Bare.on('exit', () => {
  assert(suspended, 'Should have suspended')
})
  .on('suspend', () => {
    console.log('emit suspend')
    suspended = true
    Bare.resume()
  })
  .on('idle', () => {
    console.log('emit idle')
  })
  .on('resume', () => {
    console.log('emit resume')
    assert(suspended)
  })
  .suspend()
