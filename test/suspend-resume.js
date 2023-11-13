/* global Bare */
const assert = require('bare-assert')

let suspended = false

Bare
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
