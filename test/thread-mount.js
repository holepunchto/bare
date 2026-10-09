const t = require('bare-tap')
const bundle = require('./helpers/bundle')
const { Thread } = Bare

t.plan(3)

t.throws(() => new Thread('bare:/thread.bundle', 'null', { mount: 42 }), /TypeError/)

t.throws(
  () => new Thread('bare:/thread.bundle', 'null', { mount: 'not a url' }),
  /Mount must be a URL/
)

const thread = new Thread(
  'bare:/thread.bundle',
  bundle(__filename, (filename) => {
    if (__filename !== filename) throw new Error('Module was not read at its own URL')
  }),
  { mount: 'file:///', data: __filename }
)

thread.join()
t.pass()
