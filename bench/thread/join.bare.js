const { Thread } = Bare

require('../harness')

const source = Buffer.alloc(0)

bench('new Thread().join()', () => {
  const thread = new Thread('overhead.js', { source })

  thread.join()
})
