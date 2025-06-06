import { Buffer } from 'node:buffer'

import '../harness.js'

const a = Buffer.alloc(65536, 'a')
const b = Buffer.alloc(65536, 'b')

bench('a.compare(b)', () => {
  a.compare(b)
})
