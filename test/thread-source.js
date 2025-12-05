const t = require('./harness')
const { Thread } = Bare

t.plan(2)
t.ok(Thread.isMainThread)

const entry = 'does-not-exist.js'

const thread = new Thread(entry, {
  source: Buffer.from("console.log('Hello from thread')")
})

thread.join()
t.pass()
