const t = require('bare-tap')

Bare.on('idle', onidle)

Bare.idle()

function onidle() {
  t.fail('should not idle')
}
