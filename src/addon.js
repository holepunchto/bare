/* global bare, Bare */
const { AddonError } = require('./errors')

const Addon = module.exports = exports = class Addon {
  constructor () {
    this._exports = {}
    this._handle = null

    Addon._addons.add(this)
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

      exports: this.exports
    }
  }

  static _cache = Object.create(null)
  static _addons = new Set()

  static get cache () {
    return this._cache
  }

  static get host () {
    return `${bare.platform}-${bare.arch}`
  }

  static load (url) {
    const self = Addon

    if (self._cache[url.href]) return self._cache[url.href]

    const addon = self._cache[url.href] = new Addon()

    switch (url.protocol) {
      case 'builtin:':
        addon._handle = bare.loadStaticAddon(url.pathname)
        break

      default:
        addon._handle = bare.loadDynamicAddon(url.href)
    }

    addon._exports = bare.initAddon(addon._handle, addon._exports)

    return addon
  }

  static unload (url) {
    const addon = this._cache[url.href] || null

    if (addon === null) {
      throw AddonError.ADDON_NOT_FOUND(`Cannot find addon '${url.href}'`)
    }

    const unloaded = addon.unload()

    if (unloaded) delete this._cache[url.href]

    return unloaded
  }

  static resolve (specifier, parentURL, opts = {}) {
    const Module = require('bare-module')
    const resolve = require('bare-addon-resolve')

    const self = Addon

    const {
      name = null,
      version = null,
      referrer = null,
      protocol = referrer ? referrer.protocol : null
    } = opts

    const builtins = bare.getStaticAddons()

    for (const resolution of resolve(specifier, parentURL, {
      host: self.host,
      name,
      version,
      builtins,
      extensions: [
        '.bare',
        '.node'
      ]
    }, readPackage)) {
      switch (resolution.protocol) {
        case 'builtin:': return resolution
        default:
          if (protocol.exists(resolution)) {
            return protocol.postresolve(resolution, parentURL)
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

bare.addon = function addon (specifier) {
  const name = specifier.substring(specifier.lastIndexOf('/') + 1)

  const href = 'builtin:' + name

  if (Addon._cache[href]) return Addon._cache[href]._exports

  const addon = Addon._cache[href] = new Addon()

  addon._handle = bare.loadStaticAddon(name)

  addon._exports = bare.initAddon(addon._handle, addon._exports)

  return addon._exports
}
