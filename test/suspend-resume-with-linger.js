/* global Bare */
const assert = require('bare-assert')

Bare.on('suspend', (linger) => {
  console.log('emit suspend')
  assert(linger === 1000)
})
  .on('idle', () => {
    assert(false, 'Should not idle')
  })
  .on('resume', () => {
    console.log('emit resume')
  })

Bare.suspend(1000)
Bare.resume()
