import t from 'bare-tap'

t.plan(1)

import mod from './fixtures/cjs'

t.equal(mod, 'Hello from CJS')
