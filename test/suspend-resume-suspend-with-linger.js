const assert = require('bare-assert')

let suspended = false

Bare.on('exit', () => {
  assert(suspended, 'Should have suspended')
})
  .on('suspend', (linger) => {
    console.log('emit suspend')
    assert(linger === 2000)
    suspended = true
    Bare.resume()
  })
  .on('idle', () => {
    assert(false, 'Should not idle')
  })
  .on('resume', () => {
    console.log('emit resume')
  })

Bare.suspend(1000)
Bare.resume()
Bare.suspend(2000)
