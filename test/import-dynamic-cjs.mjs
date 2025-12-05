import t from './harness'

t.plan(1)

import('./fixtures/cjs').then(({ default: mod }) => t.equal(mod, 'Hello from CJS'))
