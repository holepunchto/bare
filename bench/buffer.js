console.log('Buffer benchmark')
console.log('process.execPath =', process.execPath)

for (let n = 0; n < 3; n++) {
  console.log()
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

  {
    const a = Buffer.from('foobarz')
    const b = Buffer.from('foobary')

    console.time('smallBuf.compare(otherSmallBuf)')
    for (let i = 0; i < 1e6; i++) {
      a.compare(b)
    }
    console.timeEnd('smallBuf.compare(otherSmallBuf)')
  }

  {
    const a = Buffer.alloc(42424, 'foobarz')
    const b = Buffer.alloc(42424, 'foobary')

    console.time('bigBuf.compare(otherBigBuf)')
    for (let i = 0; i < 1e6; i++) {
      a.compare(b)
    }
    console.timeEnd('bigBuf.compare(otherBigBuf)')
  }
}
