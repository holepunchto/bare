const assert = require('assert')

let suspended = false

process
  .on('suspend', () => {
    suspended = true

    process.resume()
  })
  .on('resume', () => {
    assert(suspended)
  })
  .suspend()
