const Thread = require('thread')

const thread = new Thread({ data: Buffer.from('hello thread') }, (data) => {
  console.log(data.toString())
})

thread.join()
