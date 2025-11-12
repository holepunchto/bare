const test = require('brittle')
const path = require('bare-path')
const { Thread } = Bare

test('basic', (t) => {
  t.ok(Thread.isMainThread)

  const entry = path.join(__dirname, 'fixtures/thread.js')
  const thread = new Thread(entry, { data: Buffer.from('hello world') })
  thread.join()
})

test('thread join', (t) => {
  t.ok(Thread.isMainThread)

  const thread = new Thread(() => {})
  thread.join()
})

test('thread source', (t) => {
  t.ok(Thread.isMainThread)

  const entry = 'does-not-exist.js'
  const thread = new Thread(entry, { source: Buffer.from("console.log('Hello from thread')") })
  thread.join()
})

test('thread shared data', (t) => {
  t.ok(Thread.isMainThread)

  const entry = path.join(__dirname, 'fixtures/thread-shared-data.js')
  const data = Buffer.from(new SharedArrayBuffer(4))
  const thread = new Thread(entry, { data: data.buffer })
  thread.join()

  for (let i = 0; i < data.byteLength; i++) t.is(data[i], i + 1)
})

test('thread transfer data', (t) => {
  t.ok(Thread.isMainThread)

  const entry = path.join(__dirname, 'fixtures/thread-transfer-data.js')
  const buffer = Buffer.from('hello world')
  const thread = new Thread(entry, { data: buffer, transfer: [buffer.buffer] })
  thread.join()
})
