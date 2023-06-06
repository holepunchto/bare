const assert = require('assert')

let suspended = false

process
  .on('suspend', () => {
    console.log('emit suspend')
    suspended = true
  })
  .on('idle', () => {
    console.log('emit idle')
    process.resume()
  })
  .on('resume', () => {
    console.log('emit resume')
    assert(suspended)
  })
  .suspend()
