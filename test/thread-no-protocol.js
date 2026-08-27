const path = require('bare-path')
const t = require('bare-tap')
const bundle = require('./helpers/bundle')
const { Thread } = Bare

t.plan(2)
t.ok(Thread.isMainThread)

// A thread is loaded through a protocol that reaches nothing, so it may not read
// modules off disk even though the process that spawned it can. Whatever it
// needs travels with it as source and data.
const thread = new Thread(
  'bare:/thread.bundle',
  bundle(__filename, (filename) => {
    try {
      require(filename)

      throw new Error('Thread reached the file system')
    } catch (err) {
      if (err.code !== 'MODULE_NOT_FOUND') throw err
    }
  }),
  {
    data: path.join(__dirname, 'fixtures/thread.js')
  }
)

thread.join()
t.pass()
