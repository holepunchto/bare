const assert = require('bare-assert')

let timer

Bare.on('suspend', () => {
  console.log('emit suspend')
  timer = setTimeout(() => assert(false), 10000)
  Bare.idle()
})
  .on('idle', () => {
    console.log('emit idle')
    Bare.resume()
  })
  .on('resume', () => {
    console.log('emit resume')
    clearTimeout(timer)
  })
  .suspend()
