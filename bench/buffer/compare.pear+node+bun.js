console.log(process.execPath)

const a = Buffer.alloc(65536, 'a')
const b = Buffer.alloc(65536, 'b')

console.time('a.compare(b)')

for (let i = 0; i < 1e7; i++) {
  a.compare(b)
}

console.timeEnd('a.compare(b)')
