const Module = require('bare-module')
const resolve = require('bare-addon-resolve')
const { fileURLToPath } = require('bare-url')
const { AddonError } = require('./errors')

const { constants } = Module

module.exports = exports = class Addon {
  constructor(url) {
    this._url = url
    this._exports = {}
    this._handle = null

    Object.preventExtensions(this)

    Addon._addons.add(this)
  }

  get url() {
    return this._url
  }

  get exports() {
    return this._exports
  }

  unload() {
    let unloaded = false

    if (this._handle) {
      unloaded = bare.unloadAddon(this._handle)
      this._handle = null
    }

    Addon._addons.delete(this)

    return unloaded
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Addon },

      url: this.url,
      exports: this.exports
    }
  }

  static _protocol = Module._protocol
  static _cache = Object.create(null)
  static _addons = new Set()
  static _builtins = bare.getStaticAddons()
  static _conditions = Module._conditions

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
      addon.unload()

      delete cache[url.href]

      throw err
    }

    return addon
  }

  static unload(url, opts /* reserved */) {
    const self = Addon

    const cache = self._cache

    const addon = cache[url.href] || null

    if (addon === null) {
      throw AddonError.ADDON_NOT_FOUND(`Cannot find addon '${url.href}'`)
    }

    const unloaded = addon.unload()

    if (unloaded) delete cache[url.href]

    return unloaded
  }

  static resolve(specifier, parentURL, opts = {}) {
    const self = Addon

    const {
      referrer = null,
      protocol = referrer ? referrer._protocol : self._protocol,
      imports = referrer ? referrer._imports : null,
      resolutions = referrer ? referrer._resolutions : null,
      builtins = self._builtins,
      conditions = referrer ? referrer._conditions : self._conditions
    } = opts

    const resolved = protocol.preresolve(specifier, parentURL)

    const [resolution] = protocol.resolve(resolved, parentURL, imports)

    if (resolution) return protocol.postresolve(resolution, parentURL)

    const candidates = []

    for (const resolution of resolve(
      resolved,
      parentURL,
      {
        conditions: ['addon', ...conditions],
        host: self.host,
        resolutions,
        builtins,
        extensions: ['.bare', '.node'],
        engines: Bare.versions
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
          } catch {
            continue
          }
        default:
          if (protocol.exists(resolution, constants.types.ADDON)) {
            return protocol.postresolve(
              protocol.addon ? protocol.addon(resolution) : resolution
            )
          }
      }
    }

    let message = `Cannot find addon '${specifier}' imported from '${parentURL.href}'`

    if (candidates.length > 0) {
      message += '\nCandidates:'
      message += '\n' + candidates.map((url) => '- ' + url.href).join('\n')
    }

    throw AddonError.ADDON_NOT_FOUND(message, candidates)

    function readPackage(packageURL) {
      if (protocol.exists(packageURL, constants.types.JSON)) {
        return Module.load(packageURL, { protocol })._exports
      }

      return null
    }
  }
}
