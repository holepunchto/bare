import t from 'bare-tap'

t.plan(1)

import mod from './fixtures/mts'

t.equal(mod, 'Hello from MTS')
