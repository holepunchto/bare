/* global Bare */
const assert = require('bare-assert')

let suspended = false

Bare.on('suspend', () => {
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
  })
  .suspend()
