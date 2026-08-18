const t = require('bare-tap')
const { Addon } = Bare

t.plan(1)

const url = new URL('file:///invalid-%00-path/addon.bare')

try {
  new Addon(url)
  t.fail('constructor should have thrown')
} catch (err) {
  t.equal(err.code, 'INVALID_FILE_URL_PATH')
}
