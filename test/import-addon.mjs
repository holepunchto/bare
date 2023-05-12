import assert from 'assert'

import mod from '../build/bare_addon.bare'

assert(mod === 'Hello from addon')
