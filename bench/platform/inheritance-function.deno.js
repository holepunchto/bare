/* global bench */

import '../harness.js'

function A () {
  this.foo = 42
}

function B () {
  A.call(this)
  this.bar = 43
}

Object.setPrototypeOf(B.prototype, A.prototype)
Object.setPrototypeOf(B, A)

let r

bench('new B()', () => {
  r = new B()
})

r // eslint-disable-line no-unused-expressions
