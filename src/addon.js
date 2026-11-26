/* global bare */

const path = require('bare-path')
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

    const filename = protocol === 'file:' ? toPath(url) : url.pathname

    try {
      loader(this, filename)
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

  static get sealed() {
    return bare.addonsSealed()
  }

  static seal() {
    bare.sealAddons()
  }

  static loaded(url) {
    switch (url.protocol) {
      case 'file:':
        return bare.addonLoaded(toPath(url))
      case 'linked:':
        return bare.addonLoaded(url.pathname)
      default:
        return false
    }
  }
}

function toPath(url) {
  return path.toNamespacedPath(fileURLToPath(url))
}
