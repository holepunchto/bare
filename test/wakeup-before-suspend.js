const assert = require('bare-assert')

Bare.on('wakeup', () => {
  assert(false, 'Should not wake up')
})

Bare.wakeup(100)
