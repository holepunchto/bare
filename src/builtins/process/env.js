/* global bare */

module.exports = new Proxy(Object.create(null), {
  ownKeys (target) {
    return bare.getEnvKeys()
  },

  get (target, property) {
    return bare.getEnv(property)
  },

  has (target, property) {
    return bare.hasEnv(property)
  },

  set (target, property, value) {
    const type = typeof value

    if (type !== 'string' && type !== 'number' && type !== 'boolean') {
      throw new Error('Environment variable must be of type string, number, or boolean')
    }

    value = String(value)

    bare.setEnv(property, value)

    return true
  },

  deleteProperty (target, property) {
    bare.unsetEnv(property)
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
