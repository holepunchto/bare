/* global bare, Bare */
const { AddonError } = require('./errors')

const Addon = module.exports = exports = class Addon {
  constructor () {
    this._type = null
    this._filename = null
    this._exports = {}
    this._handle = null

    Addon._addons.add(this)
  }

  get type () {
    return this._type
  }

  get filename () {
    return this._filename
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

      type: this.type,
      filename: this.filename,
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

  static load (specifier) {
    if (this._cache[specifier]) return this._cache[specifier]._exports

    const addon = new Addon()

    try {
      addon._type = constants.types.STATIC
      addon._handle = bare.loadStaticAddon(specifier)
    } catch {
      addon._type = constants.types.DYNAMIC
      addon._filename = specifier
      addon._handle = bare.loadDynamicAddon(specifier)
    }

    addon._exports = bare.initAddon(addon._handle, addon._exports)

    this._cache[specifier] = addon

    return addon._exports
  }

  static unload (specifier) {
    const addon = this._cache[specifier] || null

    if (addon === null) {
      throw AddonError.ADDON_NOT_FOUND(`Cannot find addon '${specifier}'`)
    }

    const unloaded = addon.unload()

    if (unloaded) delete this._cache[specifier]

    return unloaded
  }

  static resolve (specifier, dirname, opts = {}) {
    const os = require('bare-os')

    if (typeof dirname !== 'string') {
      opts = dirname
      dirname = os.cwd()
    }

    const {
      referrer = null,
      protocol = referrer ? referrer.protocol : null
    } = opts

    const [resolved = null] = this._resolve(specifier, dirname, protocol)

    if (resolved === null) {
      let msg = `Cannot find addon '${specifier}'`

      if (referrer) msg += ` imported from '${referrer._filename}'`

      throw AddonError.ADDON_NOT_FOUND(msg)
    }

    return resolved
  }

  static * _resolve (specifier, dirname, protocol) {
    const path = require('bare-path')

    if (specifier[0] === '.') specifier = path.join(dirname, specifier)
    else if (path.isAbsolute(specifier)) specifier = path.normalize(specifier)

    yield * this._resolveStatic(specifier)
    yield * this._resolveDirectory(specifier, protocol)
  }

  static * _resolveStatic (specifier) {
    try {
      yield bare.resolveStaticAddon(specifier)
    } catch {}
  }

  static * _resolveFile (specifier) {
    const Module = require('bare-module')

    try {
      // We're looking specifically for a file on disk so don't pass the
      // protocol.
      yield Module.resolve(specifier)
    } catch {}
  }

  static * _resolveDirectory (specifier, protocol) {
    const Module = require('bare-module')
    const path = require('bare-path')

    let info
    try {
      info = Module.load(path.join(specifier, 'package.json'), { protocol }).exports
    } catch {
      return
    }

    const name = info.name.replace(/\//g, '+')
    const version = info.version

    const candidates = [
      `${name}.bare`,
      `${name}@${version}.bare`,
      `${name}.node`,
      `${name}@${version}.node`
    ]

    for (const directory of this._resolveAddonPaths(specifier)) {
      for (const candidate of candidates) {
        yield * this._resolveFile(path.join(directory, candidate))
      }
    }
  }

  static * _resolveAddonPaths (start) {
    const path = require('bare-path')

    const target = path.join('prebuilds', Addon.host)

    if (start === path.sep) return yield path.join(start, target)

    const parts = start.split(path.sep)

    for (let i = parts.length - 1; i >= 0; i--) {
      yield path.join(parts.slice(0, i + 1).join(path.sep), target)
    }
  }
}

const constants = exports.constants = {
  types: {
    STATIC: 1,
    DYNAMIC: 2
  }
}

Bare
  .prependListener('teardown', () => {
    for (const addon of Addon._addons) {
      addon.unload()
    }
  })

bare.addon = Addon.load.bind(Addon)
