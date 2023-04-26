/* global bench */

require('../harness')

bench('Buffer.alloc', () => {
  Buffer.alloc(65536)
})
