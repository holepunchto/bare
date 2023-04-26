/* global bench */

require('../harness')

const buffer = Buffer.alloc(65536, 'a')

bench('buffer.toString()', () => {
  buffer.toString()
})
