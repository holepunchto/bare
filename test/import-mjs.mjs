import t from 'bare-tap'

t.plan(1)

import mod from './fixtures/esm'

t.equal(mod, 'Hello from ESM')
