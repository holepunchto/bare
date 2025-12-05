const t = require('bare-tap')

t.plan(5)

Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

let idled = false
let awake = false

Bare.suspend()

function onsuspend() {
  t.pass('suspended')
}

function onidle() {
  t.pass('idled')
  if (idled++) Bare.resume()
  else Bare.wakeup(100)
}

function onresume() {
  t.pass('resumed')
}

function onwakeup() {
  t.pass('woke up')
  if (awake++) Bare.resume()
  else Bare.wakeup(100)
}
