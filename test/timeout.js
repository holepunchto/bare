const assert = require('assert')

const start = Date.now()

setTimeout(() => {
  const elapsed = Date.now() - start
  const diff = Math.abs(200 - elapsed)

  assert(diff < 5, 'Difference should be +-5 ms')
}, 200)
