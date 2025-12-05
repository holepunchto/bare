const t = require('./harness')

t.plan(5)

Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

let idled = false
let finished = false

Bare.suspend()

function onsuspend() {
  t.pass('suspended')
}

function onidle() {
  t.pass('idled')
  if (idled++) {
    t.notOk(finished, 'callback should not have run')
    Bare.resume()
  } else {
    Bare.wakeup(100)
  }
}

function onresume() {
  t.pass('resumed')
}

function onwakeup(deadline) {
  setTimeout(() => {
    finished = true
  }, deadline + 50)
}
