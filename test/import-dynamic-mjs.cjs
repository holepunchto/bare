const assert = require('bare-assert')

import('./fixtures/esm').then(({ default: mod }) => assert(mod === 'Hello from ESM'))
