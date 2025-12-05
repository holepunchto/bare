const t = require('./harness')

t.plan(3)

Bare.on('suspend', onsuspend).on('idle', onidle).on('resume', onresume).on('wakeup', onwakeup)

let timer

Bare.suspend()

function onsuspend() {
  t.pass('suspended')
  timer = setTimeout(() => t.fail('should not execute the timer callback'), 10000)
  Bare.idle()
}

function onidle() {
  t.pass('idled')
  Bare.resume()
}

function onresume() {
  t.pass('resumed')
  clearTimeout(timer)
}

function onwakeup() {
  t.fail('should not wake up')
}
