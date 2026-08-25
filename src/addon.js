/* global bare */

const { fileURLToPath } = require('bare-url')
const { AddonError } = require('./errors')

const host = bare.host

module.exports = exports = class Addon {
  constructor(url) {
    const { protocol } = url

    if (protocol !== 'builtin:' && protocol !== 'linked:' && protocol !== 'file:') {
      throw AddonError.UNKNOWN_PROTOCOL(
        `Unknown protocol '${protocol}' for addon '${url.href}'`,
        url
      )
    }

    const loader = protocol === 'builtin:' ? bare.loadStaticAddon : bare.loadDynamicAddon

    const path = protocol === 'file:' ? fileURLToPath(url) : url.pathname

    try {
      loader(this, path)
    } catch (err) {
      throw AddonError.CANNOT_LOAD(`Cannot load addon '${url.href}'`, url, err)
    }

    this._url = url
    this._exports = bare.initAddon(this, {})
  }

  get url() {
    return this._url
  }

  get exports() {
    return this._exports
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Addon },

      url: this.url,
      exports: this.exports
    }
  }

  static get host() {
    return host
  }
}
