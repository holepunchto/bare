const t = require('../harness')
const { Thread } = Bare

t.plan(2)
t.ok(Thread.isMainThread)
t.ok(Thread.self.data.equals(Buffer.from('hello world')))
