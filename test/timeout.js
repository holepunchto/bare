const assert = require('assert')

const start = Date.now()

setTimeout(() => {
  const elapsed = Date.now() - start
  const diff = Math.abs(200 - elapsed)

  assert(diff < 50, 'Difference should be +-50 ms')
}, 200)
