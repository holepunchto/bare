/* global bench */

import { Buffer } from 'node:buffer'

import '../harness.js'

const buffer = Buffer.alloc(65536, 'a')

bench('buffer.toString()', () => {
  buffer.toString()
})
