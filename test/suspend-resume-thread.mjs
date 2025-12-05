import t from './harness'
const { Thread } = Bare

const thread = new Thread(() => {
  Bare.on('suspend', () => console.log('suspended'))
    .on('idle', () => console.log('idled'))
    .on('resume', () => {
      console.log('resumed')
      Bare.off('exit', onexit)
    })
    .on('exit', onexit)

  function onexit() {
    assert(false, 'should not exit')
  }
})

thread.suspend()
await t.sleep(100)

thread.resume()
thread.join()
