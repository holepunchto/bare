const assert = require('bare-assert')

let suspended = false
let idled = false
let finished = false

Bare.on('exit', () => {
  assert(suspended, 'Should have suspended')
  assert(idled, 'Should have idled')
  assert(finished, 'Callback should have run')
})
  .on('suspend', () => {
    console.log('emit suspend')
    suspended = true
  })
  .on('idle', () => {
    console.log('emit idle')
    if (idled) {
      assert(!finished, 'Callback should not have run')
      Bare.resume()
    } else {
      idled = true
      Bare.wakeup(100)
    }
  })
  .on('resume', () => {
    console.log('emit resume')
    assert(suspended)
  })
  .on('wakeup', (deadline) => {
    console.log('emit wakeup')
    setTimeout(() => {
      finished = true
    }, deadline + 50)
  })

Bare.suspend()
