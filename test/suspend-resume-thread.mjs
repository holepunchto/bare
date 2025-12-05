import t from 'bare-tap'
const { Thread } = Bare

t.plan(1)

const thread = new Thread(import.meta.url, async () => {
  const { default: t } = await import('bare-tap')

  t.plan(4)

  let resumed = false

  Bare.on('suspend', onsuspend)
    .on('idle', onidle)
    .on('resume', onresume)
    .prependListener('exit', onexit)

  function onsuspend() {
    t.pass('suspended')
  }

  function onidle() {
    t.pass('idled')
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

thread.resume()
thread.join()
t.pass()
