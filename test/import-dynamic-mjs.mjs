import t from 'bare-tap'

t.plan(1)

import('./fixtures/esm').then(({ default: mod }) => t.equal(mod, 'Hello from ESM'))
