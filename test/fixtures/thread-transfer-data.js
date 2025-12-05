const t = require('../harness')
const { Thread } = Bare

t.plan(2)
t.ok(Thread.isMainThread)

const data = Thread.self.data

t.equal(data.toString(), 'hello world')
