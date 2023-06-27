const assert = require('assert')

let suspended = false

process
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

process.suspend()
process.resume()
