/* global bench */

require('../harness')

class A {
  constructor () {
    this.foo = 42
  }
}

class B extends A {
  constructor () {
    super()
    this.bar = 43
  }
}

let r

bench('new B()', () => {
  r = new B()
})

r // eslint-disable-line no-unused-expressions
