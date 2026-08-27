const path = require('bare-path')
const t = require('bare-tap')
const bundle = require('./helpers/bundle')
const { Thread } = Bare

t.plan(2)
t.ok(Thread.isMainThread)

const entry = path.join(__dirname, 'fixtures/thread.js')

const thread = new Thread('bare:/thread.bundle', bundle(entry), {
  data: Buffer.from('hello world')
})

thread.join()
t.pass()
