require('../harness')

bench('new URL()', () => {
  new URL('https://example.com/hello/world?query=string#fragment')
})
