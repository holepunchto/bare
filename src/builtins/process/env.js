/* global pear */

module.exports = new Proxy(Object.create(null), {
  ownKeys (target) {
    return pear.getEnvKeys()
  },

  get (target, property) {
    return pear.getEnv(property)
  },

  has (target, property) {
    return pear.hasEnv(property)
  },

  set (target, property, value) {
    const type = typeof value

    if (type !== 'string' && type !== 'number' && type !== 'boolean') {
      throw new Error('Environment variable must be of type string, number, or boolean')
    }

    value = String(value)

    pear.setEnv(property, value)
  },

  deleteProperty (target, property) {
    pear.unsetEnv(property)
  },

  getOwnPropertyDescriptor (target, property) {
    return {
      value: this.get(target, property),
      enumerable: true,
      configurable: true,
      writable: true
    }
  }
})
