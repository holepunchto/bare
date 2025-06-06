require('../harness')

const object = {
  string: 'hello world',
  number: 42,
  boolean: true,
  array: ['element'],
  object: {
    key: 'value'
  }
}

bench('structuredClone()', () => {
  structuredClone(object)
})
