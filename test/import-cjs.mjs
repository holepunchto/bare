import t from './harness'

t.plan(1)

import mod from './fixtures/cjs'

t.equal(mod, 'Hello from CJS')
