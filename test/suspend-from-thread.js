const assert = require('assert')
const Thread = require('thread')

process
  .on('suspend', () => {
    console.log('emit suspend')
    process.resume()
  })
  .on('idle', () => {
    assert(false, 'Should not idle')
  })
  .on('resume', () => {
    console.log('emit resume')
  })

const thread = new Thread(() => process.suspend())

thread.join()
