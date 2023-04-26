import process from 'node:process'
import { Buffer } from 'node:buffer'

console.log(process.execPath)

const buffer = Buffer.alloc(65536, 'a')

console.time('buffer.toString()')

for (let i = 0; i < 1e6; i++) {
  buffer.toString()
}

console.timeEnd('buffer.toString()')
