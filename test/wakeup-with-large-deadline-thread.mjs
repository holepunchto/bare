import t from 'bare-tap'
import bundle from './helpers/bundle.js'
const { Thread } = Bare

t.plan(3)

const ready = new Int32Array(new SharedArrayBuffer(4))

const thread = new Thread(
  'bare:/thread.bundle',
  bundle(new URL(import.meta.url), async () => {
    const { default: tap } = await import('bare-tap')

    const ready = new Int32Array(Bare.Thread.self.data)

    const t = tap.subtest()

    t.plan(2)

    Bare.on('suspend', onsuspend).on('wakeup', onwakeup)

    Atomics.store(ready, 0, 1)
    Atomics.notify(ready, 0)

    Atomics.wait(ready, 0, 1)

    function onsuspend(linger) {
      t.equal(linger, 0x7fffffff, 'suspend accepts the limit')
    }

    function onwakeup(deadline) {
      t.equal(deadline, 0x7fffffff, 'wakeup accepts the limit')
    }
  }),
  {
    mount: bundle.mount,
    data: ready.buffer
  }
)

Atomics.wait(ready, 0, 0)

t.throws(() => thread.suspend(2 ** 32 + 1000), RangeError, 'suspend throws instead of wrapping')
t.throws(() => thread.wakeup(3_000_000_000), RangeError, 'wakeup throws instead of wrapping')

thread.suspend(0x7fffffff)

Atomics.store(ready, 0, 2)
Atomics.notify(ready, 0)

await t.sleep(500)

thread.wakeup(0x7fffffff)
await t.sleep(500)

thread.resume()
thread.join()
t.pass()
