const resolve = require('bare-addon-resolve')
const { fileURLToPath } = require('bare-url')
const { AddonError } = require('./errors')

module.exports = exports = class Addon {
  constructor(url) {
    this._url = url
    this._exports = {}
    this._handle = null

    Object.preventExtensions(this)
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

  static _cache = Object.create(null)

  static _builtins = bare.getStaticAddons()

  static get cache() {
    return this._cache
  }

  static get host() {
    return bare.host
  }

  static load(url, opts /* reserved */) {
    const self = Addon

    const cache = self._cache

    let addon = cache[url.href] || null

    if (addon !== null) return addon

    addon = cache[url.href] = new Addon(url)

    try {
      switch (url.protocol) {
        case 'builtin:':
          addon._handle = bare.loadStaticAddon(url.pathname)
          break
        case 'linked:':
          addon._handle = bare.loadDynamicAddon(url.pathname)
          break
        case 'file:':
          addon._handle = bare.loadDynamicAddon(fileURLToPath(url))
          break
        default:
          throw AddonError.UNSUPPORTED_PROTOCOL(
            `Unsupported protocol '${url.protocol}' for addon '${url.href}'`
          )
      }

      addon._exports = bare.initAddon(addon._handle, addon._exports)
    } catch (err) {
      delete cache[url.href]

      throw err
    }

    return addon
  }

  static resolve(specifier, parentURL, opts = {}) {
    const Module = require('bare-module')

    const self = Addon

    if (typeof specifier !== 'string') {
      throw new TypeError(
        `Specifier must be a string. Received type ${typeof specifier} (${specifier})`
      )
    }

    const {
      referrer = null,
      protocol = referrer ? referrer._protocol : Module._protocol,
      imports = referrer ? referrer._imports : null,
      resolutions = referrer ? referrer._resolutions : null,
      builtins = self._builtins,
      conditions = referrer ? referrer._conditions : Module._conditions
    } = opts

    const resolved = protocol.preresolve(specifier, parentURL)

    const [resolution] = protocol.resolve(resolved, parentURL, imports)

    if (resolution) return protocol.postresolve(resolution, parentURL)

    const candidates = []

    let cause

    for (const resolution of resolve(
      resolved,
      parentURL,
      {
        conditions: ['addon', ...conditions],
        host: self.host,
        resolutions,
        builtins,
        extensions: ['.bare', '.node'],
        engines: bare.versions
      },
      readPackage
    )) {
      candidates.push(resolution)

      switch (resolution.protocol) {
        case 'builtin:':
          return resolution
        case 'linked:':
          try {
            return Addon.load(resolution, opts).url
          } catch (err) {
            cause = err
            break
          }
        default:
          if (protocol.exists(resolution, Module.constants.types.ADDON)) {
            return protocol.postresolve(protocol.addon ? protocol.addon(resolution) : resolution)
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
      if (protocol.exists(packageURL, Module.constants.types.JSON)) {
        return Module.load(packageURL, { protocol })._exports
      }

      return null
    }
  }
}
