/* global bare */

const resolve = require('bare-addon-resolve')
const { fileURLToPath } = require('bare-url')
const { AddonError } = require('./errors')

const engines = bare.versions
const host = bare.host
const builtins = bare.getStaticAddons()

const conditions = ['bare', 'node', 'addon', ...host.split('-')]
const extensions = ['.bare', '.node']
const cache = Object.create(null)

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

  static get sealed() {
    return bare.addonsSealed()
  }

  static seal() {
    bare.sealAddons()
  }

  /** @deprecated */
  static get cache() {
    return cache
  }

  /** @deprecated */
  static load(url) {
    let addon = cache[url.href] || null

    if (addon !== null) return addon

    addon = cache[url.href] = new Addon(url)

    return addon
  }

  /** @deprecated */
  static resolve(specifier, parentURL, opts = {}) {
    const defaultProtocol = require('./protocol')

    if (typeof specifier !== 'string') {
      throw new TypeError(
        `Specifier must be a string. Received type ${typeof specifier} (${specifier})`
      )
    }

    const {
      referrer = null,
      protocol = referrer ? referrer.protocol : defaultProtocol,
      resolutions = referrer ? referrer.resolutions : null
    } = opts

    const candidates = []

    let cause

    for (const resolution of resolve(
      specifier,
      parentURL,
      { host, builtins, resolutions, conditions, extensions, engines },
      readPackage
    )) {
      candidates.push(resolution)

      switch (resolution.protocol) {
        case 'builtin:':
          return resolution
        case 'linked:':
          try {
            return Addon.load(resolution).url
          } catch (err) {
            cause = err
            break
          }
        default:
          if (defaultProtocol.exists(resolution)) {
            return defaultProtocol.postresolve(resolution)
          }
      }
    }

    let message = `Cannot find addon '${specifier}' imported from '${parentURL.href}'`

    if (candidates.length > 0) {
      message += '\nCandidates:'
      message += '\n' + candidates.map((url) => '- ' + url.href).join('\n')
    }

    throw AddonError.ADDON_NOT_FOUND(message, specifier, parentURL, candidates, cause)

    function readPackage(packageURL) {
      if (protocol.exists(packageURL)) {
        const source = protocol.read(packageURL)

        try {
          return JSON.parse(source)
        } catch {
          return null
        }
      }

      return null
    }
  }
}
