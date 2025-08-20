const assert = require('bare-assert')

let suspended = false
let idle = false

Bare.on('exit', () => {
  assert(idle, 'Should have idled')
})
  .on('suspend', () => {
    console.log('emit suspend')
    suspended = true
  })
  .on('idle', () => {
    console.log('emit idle')
    idle = true
    Bare.resume()
  })
  .on('resume', () => {
    console.log('emit resume')
    assert(suspended)
  })

Bare.suspend()
Bare.resume()
Bare.suspend()
