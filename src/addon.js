/* global bare, Bare */

const { AddonError } = require('./errors')

module.exports = exports = class Addon {
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

  static load (specifier) {
    if (this._cache[specifier]) return this._cache[specifier]._exports

    const addon = new Addon()

    try {
      addon._type = constants.types.STATIC
      addon._handle = bare.loadStaticAddon(specifier)
    } catch {
      specifier = this.resolve(specifier)

      if (this._cache[specifier]) return this._cache[specifier]._exports

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

  static resolve (specifier) {
    const [resolved = null] = this._resolve(specifier)

    if (resolved === null) {
      throw AddonError.ADDON_NOT_FOUND(`Cannot find addon '${specifier}'`)
    }

    return resolved
  }

  static * _resolve (specifier) {
    yield * this._resolveFile(specifier)
    yield * this._resolveDirectory(specifier)
  }

  static * _resolveFile (specifier) {
    const path = require('bare-path')

    switch (path.extname(specifier)) {
      case '.bare':
      case '.node':
        yield specifier
    }
  }

  static * _resolveDirectory (specifier) {
    const path = require('bare-path')
    const Module = require('bare-module')

    let info = null
    try {
      info = Module.load(path.join(specifier, 'package.json')).exports
    } catch {
      const nodeModules = path.normalize('/node_modules/')

      const i = specifier.indexOf(nodeModules)

      if (i !== -1) {
        specifier = specifier.substring(i + nodeModules.length)

        try {
          info = Module.load(Module.resolve(path.join(specifier, 'package.json'))).exports
        } catch {}
      }
    }

    if (info) {
      try {
        specifier = path.dirname(Module.resolve(path.join(info.name, 'package.json')))
      } catch {}
    }

    for (const directory of this._resolveAddonPaths(specifier)) {
      try {
        const files = bare.readdir(directory)

        for (const file of files) {
          yield * this._resolveFile(path.join(directory, file))
        }
      } catch {}
    }
  }

  static * _resolveAddonPaths (specifier) {
    const path = require('bare-path')
    const os = require('bare-os')

    yield path.join(specifier, 'build/Debug')
    yield path.join(specifier, 'build/Release')
    yield path.join(specifier, 'build')
    yield path.join(specifier, 'prebuilds', `${os.platform()}-${os.arch()}`)
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
    for (const addon of exports._addons) {
      addon.unload()
    }
  })
