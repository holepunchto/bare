/* global pear */

module.exports = new Proxy(pear.env(Object.create(null)), {
  get (target, property) {
    return target[property]
  },

  set (target, property, value) {
    const type = typeof value

    if (type !== 'string' && type !== 'number' && type !== 'boolean') {
      throw new Error('Environment variable must be of type string, number, or boolean')
    }

    value = String(value)

    pear.setEnv(property, value)

    target[property] = value
  },

  deleteProperty (target, property) {
    pear.unsetEnv(property)

    delete target[property]
  }
})
