import process from 'node:process'
import { Buffer } from 'node:buffer'

console.log(process.execPath)

console.time('Buffer.allocUnsafe')

for (let i = 0; i < 1e7; i++) {
  Buffer.allocUnsafe(65536)
}

console.timeEnd('Buffer.allocUnsafe')
