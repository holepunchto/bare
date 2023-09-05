/* global bare */

const { AddonError } = require('./errors')

module.exports = class Addon {
  constructor () {
    this.filename = null
    this.exports = {}

    this._type = null
    this._handle = null
  }

  unload () {
    return bare.unloadAddon(this._handle)
  }

  static _path = null
  static _cache = Object.create(null)

  static get path () {
    return this._path
  }

  static set path (value) {
    this._path = value
  }

  static get cache () {
    return this._cache
  }

  static load (specifier) {
    if (this._cache[specifier]) return this._cache[specifier].exports

    const addon = new Addon()

    try {
      addon._handle = bare.loadStaticAddon(specifier)
      addon._type = 'static'
    } catch {
      specifier = this.resolve(specifier)

      if (this._cache[specifier]) return this._cache[specifier].exports

      addon.filename = specifier

      addon._handle = bare.loadDynamicAddon(specifier)
      addon._type = 'dynamic'
    }

    addon.exports = bare.initAddon(addon._handle, addon.exports)

    this._cache[specifier] = addon

    return addon.exports
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
    const path = require('./path')

    switch (path.extname(specifier)) {
      case '.bare':
      case '.node':
        yield specifier
    }
  }

  static * _resolveDirectory (specifier) {
    const Module = require('./module')
    const path = require('./path')

    const protocol = Module._protocols['file:']

    const pkg = path.join(specifier, 'package.json')

    let info
    try {
      info = Module.load(pkg).exports
    } catch {
      info = null
    }

    if (info) {
      if (this._path) {
        const name = info.name.replace(/\//g, '+')
        const version = info.version

        for (const candidate of [
          `${name}.bare`,
          `${name}@${version}.bare`,
          `${name}.node`,
          `${name}@${version}.node`
        ]) {
          const file = path.join(this._path, candidate)

          try {
            if (protocol.exists(file)) yield file
          } catch {}
        }
      }

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
    const path = require('./path')

    yield path.join(specifier, 'build/Debug')
    yield path.join(specifier, 'build/Release')
    yield path.join(specifier, 'build')
    yield path.join(specifier, 'prebuilds', `${process.platform}-${process.arch}`)
  }
}
