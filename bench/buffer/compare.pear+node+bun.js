/* global bench */

require('../harness')

const a = Buffer.alloc(65536, 'a')
const b = Buffer.alloc(65536, 'b')

bench('a.compare(b)', () => {
  a.compare(b)
})
