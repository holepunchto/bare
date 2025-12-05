const t = require('bare-tap')

t.plan(1)

import('./fixtures/cjs').then(({ default: mod }) => t.equal(mod, 'Hello from CJS'))
