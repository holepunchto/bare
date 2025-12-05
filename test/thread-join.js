const t = require('./harness')
const { Thread } = Bare

t.plan(2)
t.ok(Thread.isMainThread)

const thread = new Thread(() => {})

thread.join()
t.pass()
