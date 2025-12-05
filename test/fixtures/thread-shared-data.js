const t = require('bare-tap')
const { Thread } = Bare

t.plan(2)
t.notOk(Thread.isMainThread)

const data = Buffer.from(Thread.self.data)

for (let i = 0; i < data.byteLength; i++) {
  data[i] = i + 1
}

t.pass()
