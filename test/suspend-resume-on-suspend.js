/* global Bare */
const assert = require('bare-assert')

let suspended = false

Bare
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
