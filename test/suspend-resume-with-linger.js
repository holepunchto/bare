const assert = require('bare-assert')

let suspended = false

Bare.on('exit', () => {
  assert(suspended, 'Should have suspended')
})
  .on('suspend', (linger) => {
    console.log('emit suspend')
    assert(linger === 1000)
    suspended = true
  })
  .on('idle', () => {
    assert(false, 'Should not idle')
  })
  .on('resume', () => {
    console.log('emit resume')
  })

Bare.suspend(1000)
Bare.resume()
