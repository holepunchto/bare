/* global bench */

import { Buffer } from 'node:buffer'

import '../harness.js'

const buffer = Buffer.alloc(65536, 'a')

bench('buffer.toString(\'utf8\')', () => {
  buffer.toString()
})

bench('buffer.toString(\'utf16le\')', () => {
  buffer.toString('utf16le')
})

bench('buffer.toString(\'hex\')', () => {
  buffer.toString('hex')
})

bench('buffer.toString(\'base64\')', () => {
  buffer.toString('base64')
})
