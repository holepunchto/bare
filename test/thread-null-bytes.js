const t = require('bare-tap')
const { Thread } = Bare

t.plan(1)

t.throws(() => new Thread('bare:/thread.bundle\0.js', 'null'), /null bytes/)
