const t = require('bare-tap')
const { Thread } = Bare

t.plan(2)
t.notOk(Thread.isMainThread)

const data = Thread.self.data

t.equal(data.toString(), 'hello world')
