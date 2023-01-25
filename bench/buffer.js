for (let n = 0; n < 2; n++) {
  if (n) console.log()
  console.log('Round ' + n)

  console.time('Buffer.allocUnsafe')
  for (let i = 0; i < 1e6; i++) {
    Buffer.allocUnsafe(65536)
  }
  console.timeEnd('Buffer.allocUnsafe')

  console.time('Buffer.alloc')
  for (let i = 0; i < 1e6; i++) {
    Buffer.alloc(65536)
  }
  console.timeEnd('Buffer.alloc')

  const a = Buffer.from('foobarz')
  const b = Buffer.from('foobary')

  console.time('buf.compare(otherBuf)')
  for (let i = 0; i < 1e6; i++) {
    a.compare(b)
  }
  console.timeEnd('buf.compare(otherBuf)')
}
