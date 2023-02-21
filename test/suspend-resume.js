const assert = require('assert')

const start = Date.now()

process
  .on('suspend', () => {
    const timer = setTimeout(() => process.resume(), 200)
    timer.unref()
  })
  .on('resume', () => {
    const elapsed = Date.now() - start
    const diff = Math.abs(200 - elapsed)

    assert(diff < 5, 'Difference should be +-5 ms')
  })
  .suspend()
