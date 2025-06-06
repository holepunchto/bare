require('../harness')

bench('Buffer.allocUnsafe', () => {
  Buffer.allocUnsafe(65536)
})
