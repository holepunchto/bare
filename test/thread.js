const path = require('bare-path')
const t = require('bare-tap')
const { Thread } = Bare

t.plan(2)
t.ok(Thread.isMainThread)

const entry = path.join(__dirname, 'fixtures/thread.js')

const thread = new Thread(entry, { data: Buffer.from('hello world') })

thread.join()
t.pass()
