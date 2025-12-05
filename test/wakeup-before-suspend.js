const t = require('bare-tap')

Bare.on('wakeup', onwakeup)

Bare.wakeup(100)

function onwakeup() {
  t.fail('should not wake up')
}
