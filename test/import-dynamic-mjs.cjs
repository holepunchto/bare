const t = require('bare-tap')

t.plan(1)

import('./fixtures/mjs').then(({ default: mod }) => t.equal(mod, 'Hello from ESM'))
