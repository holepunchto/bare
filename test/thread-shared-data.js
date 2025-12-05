const path = require('bare-path')
const t = require('./harness')
const { Thread } = Bare

t.plan(6)
t.ok(Thread.isMainThread)

const entry = path.join(__dirname, 'fixtures/thread-shared-data.js')

const data = Buffer.from(new SharedArrayBuffer(4))

const thread = new Thread(entry, { data: data.buffer })

thread.join()
t.pass()

for (let i = 0; i < data.byteLength; i++) {
  t.equal(data[i], i + 1)
}
