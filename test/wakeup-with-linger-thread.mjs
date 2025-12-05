import t from 'bare-tap'
const { Thread } = Bare

t.plan(1)

const thread = new Thread(import.meta.url, async () => {
  const { default: tap } = await import('bare-tap')

  const t = tap.subtest()

  t.plan(6)

  let resumed = false

  Bare.on('suspend', onsuspend)
    .on('idle', onidle)
    .on('wakeup', onwakeup)
    .on('resume', onresume)
    .prependListener('exit', onexit)

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

thread.suspend()
await t.sleep(100)

thread.wakeup(1000)
await t.sleep(100)

thread.resume()
thread.join()
t.pass()
