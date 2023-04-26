globalThis.bench = function bench (name, fn) {
  let elapsed = 0
  let operations = 0

  const start = Date.now()

  while (elapsed < 4000) {
    fn()

    elapsed = Date.now() - start
    operations++
  }

  console.log(name, operations / elapsed * 1e3 | 0, 'ops/s')
}
