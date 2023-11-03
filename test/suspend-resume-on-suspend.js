const assert = require('assert')

let suspended = false

process
  .on('suspend', () => {
    console.log('emit suspend')
    suspended = true
    process.resume()
  })
  .on('idle', () => {
    console.log('emit idle')
  })
  .on('resume', () => {
    console.log('emit resume')
    assert(suspended)
  })
  .suspend()
