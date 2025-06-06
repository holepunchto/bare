import { Buffer } from 'node:buffer'

import '../harness.js'

bench('Buffer.alloc', () => {
  Buffer.alloc(65536)
})
