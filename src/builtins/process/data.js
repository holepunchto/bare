/* global pear */

module.exports = new Proxy(pear.data, {
  get (target, property) {
    return target[property] || null
  },

  set (target, property, value) {
    target[property] = value
  },

  deleteProperty (target, property) {
    delete target[property]
  }
})
