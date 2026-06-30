import t from 'bare-tap'

t.plan(1)

import mod from './fixtures/mjs'

t.equal(mod, 'Hello from ESM')
