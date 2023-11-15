const assert = require('bare-assert')

const id = setTimeout(() => {
  assert(false, 'Should have been cancelled')
}, 200)

clearTimeout(id)
