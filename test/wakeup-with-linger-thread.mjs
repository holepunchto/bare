import t from 'bare-tap'
const { Thread } = Bare

t.plan(1)

const ready = new Int32Array(new SharedArrayBuffer(4))

const thread = new Thread(import.meta.url, { data: ready.buffer }, async () => {
  const { default: tap } = await import('bare-tap')

  const ready = new Int32Array(Bare.Thread.self.data)

  const t = tap.subtest()

  t.plan(6)

  let resumed = false

  Bare.on('suspend', onsuspend)
    .on('idle', onidle)
    .on('wakeup', onwakeup)
    .on('resume', onresume)
    .prependListener('exit', onexit)

  Atomics.store(ready, 0, 1)
  Atomics.notify(ready, 0)

  Atomics.wait(ready, 0, 1)

  function onsuspend() {
    t.pass('suspended')
  }

  function onidle() {
    t.pass('idled')
  }

  function onwakeup() {
    t.pass('woke up')
  }

  function onresume() {
    t.pass('resumed')
    resumed = true
  }

  function onexit() {
    t.ok(resumed, 'resumed before exit')
  }
})

Atomics.wait(ready, 0, 0)

thread.suspend()

Atomics.store(ready, 0, 2)
Atomics.notify(ready, 0)

await t.sleep(500)

thread.wakeup(1000)
await t.sleep(500)

thread.resume()
thread.join()
t.pass()
