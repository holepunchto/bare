import t from 'bare-tap'
import bundle from './helpers/bundle.js'
const { Thread } = Bare

t.plan(2)

const shared = new Int32Array(new SharedArrayBuffer(12))

const thread = new Thread(
  'bare:/thread.bundle',
  bundle(new URL(import.meta.url), () => {
    const shared = new Int32Array(Bare.Thread.self.data)

    Bare.on('suspend', onsuspend).on('wakeup', onwakeup)

    Atomics.store(shared, 0, 1)
    Atomics.notify(shared, 0)

    Atomics.wait(shared, 0, 1)

    function onsuspend(linger) {
      Atomics.store(shared, 1, linger)
    }

    function onwakeup(deadline) {
      Atomics.store(shared, 2, deadline)
    }
  }),
  {
    mount: bundle.mount,
    data: shared.buffer
  }
)

Atomics.wait(shared, 0, 0)

thread.suspend(2 ** 32 + 1000)

Atomics.store(shared, 0, 2)
Atomics.notify(shared, 0)

await t.sleep(500)

thread.wakeup(3_000_000_000)
await t.sleep(500)

thread.resume()
thread.join()

t.equal(Atomics.load(shared, 1), 0x7fffffff, 'linger is clamped')
t.equal(Atomics.load(shared, 2), 0x7fffffff, 'deadline is clamped')
