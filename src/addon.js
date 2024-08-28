/* global bare, Bare */
const Module = require('bare-module')
const resolve = require('bare-addon-resolve')
const { fileURLToPath } = require('bare-url')
const { AddonError } = require('./errors')

const Addon = module.exports = exports = class Addon {
  constructor (url) {
    this._url = url
    this._exports = {}
    this._handle = null

    Addon._addons.add(this)
  }

  get url () {
    return this._url
  }

  get exports () {
    return this._exports
  }

  unload () {
    let unloaded = false

    if (this._handle) {
      unloaded = bare.unloadAddon(this._handle)
      this._handle = null
    }

    Addon._addons.delete(this)

    return unloaded
  }

  [Symbol.for('bare.inspect')] () {
    return {
      __proto__: { constructor: Addon },

      url: this.url,
      exports: this.exports
    }
  }

  static _cache = Object.create(null)
  static _addons = new Set()

  static get cache () {
    return this._cache
  }

  static get host () {
    return `${bare.platform}-${bare.arch}${bare.simulator ? '-simulator' : ''}`
  }

  static load (url) {
    const self = Addon

    const cache = self._cache

    if (cache[url.href]) return cache[url.href]

    const addon = cache[url.href] = new Addon(url)

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
          throw AddonError.UNSUPPORTED_PROTOCOL(`Unsupported protocol for addon ${url.href}`)
      }

      addon._exports = bare.initAddon(addon._handle, addon._exports)
    } catch (err) {
      addon.unload()

      delete cache[url.href]

      throw err
    }

    return addon
  }

  static unload (url) {
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

  static resolve (specifier, parentURL, opts = {}) {
    const self = Addon

    const {
      name = null,
      version = null,
      referrer = null,
      protocol = referrer ? referrer._protocol : Module._protocols['file:'],
      imports = referrer ? referrer._imports : null,
      resolutions = referrer ? referrer._resolutions : null
    } = opts

    const builtins = bare.getStaticAddons()

    const resolved = protocol.preresolve(specifier, parentURL)

    const [resolution] = protocol.resolve(specifier, parentURL, imports)

    if (resolution) return protocol.postresolve(resolution, parentURL)

    for (const resolution of resolve(resolved, parentURL, {
      host: self.host,
      name,
      version,
      resolutions,
      builtins,
      extensions: [
        '.bare',
        '.node'
      ]
    }, readPackage)) {
      switch (resolution.protocol) {
        case 'builtin:': return resolution

        case 'linked:':
          try {
            return Addon.load(resolution).url
          } catch {
            continue
          }

        case 'file:':
          try {
            return Module.resolve(resolution.href, parentURL, { imports, resolutions })
          } catch {
            continue
          }
      }
    }

    let msg = `Cannot find addon '${specifier}'`

    if (referrer) msg += ` imported from '${referrer._url.href}'`

    throw AddonError.ADDON_NOT_FOUND(msg)

    function readPackage (packageURL) {
      if (protocol.exists(packageURL)) {
        return Module.load(packageURL, { protocol })._exports
      }

      return null
    }
  }
}

Bare
  .prependListener('teardown', () => {
    for (const addon of Addon._addons) {
      addon.unload()
    }
  })
