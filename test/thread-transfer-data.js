const path = require('bare-path')
const t = require('bare-tap')
const bundle = require('./helpers/bundle')
const { Thread } = Bare

t.plan(2)
t.ok(Thread.isMainThread)

const entry = path.join(__dirname, 'fixtures/thread-transfer-data.js')

const buffer = Buffer.from('hello world')

const thread = new Thread('bare:/thread.bundle', bundle(entry), {
  data: buffer,
  transfer: [buffer.buffer]
})

thread.join()
t.pass()
