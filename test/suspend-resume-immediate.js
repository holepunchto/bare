const assert = require('assert')

let suspended = false

process
  .on('suspend', () => {
    suspended = true

    process.resume()
  })
  .on('idle', () => {
    assert(false, 'Should not idle')
  })
  .on('resume', () => {
    assert(suspended)
  })
  .suspend()
