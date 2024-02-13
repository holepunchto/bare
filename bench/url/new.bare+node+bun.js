/* global bench */

require('../harness')

bench('new URL()', () => {
  new URL('https://example.com/hello/world?query=string#fragment') // eslint-disable-line no-new
})
