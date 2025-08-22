const assert = require('bare-assert')

let suspended = 0
let idle = false

Bare.on('exit', () => {
  assert(idle, 'Should have idled')
  assert(suspended === 2, 'Should have suspended 2 times')
})
  .on('suspend', () => {
    console.log('emit suspend')
    if (suspended++) return
    Bare.resume()
    Bare.suspend()
  })
  .on('idle', () => {
    console.log('emit idle')
    idle = true
    Bare.resume()
  })
  .on('resume', () => {
    console.log('emit resume')
  })

Bare.suspend()
