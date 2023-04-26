const Thread = require('thread')

const source = Buffer.alloc(0)

console.time('new Thread().join()')

for (let i = 0; i < 1000; i++) {
  const thread = new Thread('overhead.js', { source })

  thread.join()
}

console.timeEnd('new Thread().join()')
