/* global bare */

const resolve = require('bare-addon-resolve')
const { fileURLToPath } = require('bare-url')
const { AddonError } = require('./errors')

const host = bare.host
const builtins = bare.getStaticAddons()

const defaultConditions = ['bare', 'node', ...host.split('-')]
const defaultCache = Object.create(null)

module.exports = exports = class Addon {
  constructor(url) {
    this._url = url
    this._exports = {}
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

  static get cache() {
    return defaultCache
  }

  static get host() {
    return host
  }

  static load(url, opts /* reserved */) {
    let addon = defaultCache[url.href] || null

    if (addon !== null) return addon

    addon = defaultCache[url.href] = new Addon(url)

    try {
      switch (url.protocol) {
        case 'builtin:':
          bare.loadStaticAddon(addon, url.pathname)
          break
        case 'linked:':
          bare.loadDynamicAddon(addon, url.pathname)
          break
        case 'file:':
          bare.loadDynamicAddon(addon, fileURLToPath(url))
          break
        default:
          throw AddonError.UNSUPPORTED_PROTOCOL(
            `Unsupported protocol '${url.protocol}' for addon '${url.href}'`
          )
      }

      addon._exports = bare.initAddon(addon, addon._exports)
    } catch (err) {
      delete defaultCache[url.href]

      throw err
    }

    return addon
  }

  static resolve(specifier, parentURL, opts = {}) {
    const Module = require('bare-module')

    if (typeof specifier !== 'string') {
      throw new TypeError(
        `Specifier must be a string. Received type ${typeof specifier} (${specifier})`
      )
    }

    const {
      referrer = null,
      protocol = referrer ? referrer.protocol : Module.protocol,
      imports = referrer ? referrer.imports : null,
      resolutions = referrer ? referrer.resolutions : null,
      conditions = referrer ? referrer.conditions : defaultConditions
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
        host,
        builtins,
        resolutions,
        conditions: ['addon', ...conditions],
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
        return Module.load(packageURL, { protocol, referrer }).exports
      }

      return null
    }
  }
}
