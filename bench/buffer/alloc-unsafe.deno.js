import { Buffer } from 'node:buffer'

import '../harness.js'

bench('Buffer.allocUnsafe', () => {
  Buffer.allocUnsafe(65536)
})
