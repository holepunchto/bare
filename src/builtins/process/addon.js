/* global bare */

module.exports = exports = function addon (specifier) {
  if (exports.cache[specifier]) return exports.cache[specifier]

  let addon = null

  try {
    addon = bare.loadStaticAddon(specifier)
  } catch {}

  if (addon === null) {
    specifier = resolve(specifier)

    if (exports.cache[specifier]) return exports.cache[specifier]

    addon = bare.loadDynamicAddon(specifier)
  }

  return (exports.cache[specifier] = addon)
}

exports.cache = Object.create(null)

exports.path = null

const resolve = exports.resolve = function resolve (specifier) {
  const Module = require('../module')
  const path = require('../path')

  const [resolved = null] = resolve(specifier)

  if (resolved === null) {
    throw new Error(`Cannot resolve addon '${specifier}'`)
  }

  return resolved

  function * resolve (specifier) {
    yield * resolveFile(specifier)
    yield * resolveDirectory(specifier)
  }

  function * resolveFile (specifier) {
    switch (path.extname(specifier)) {
      case '.bare':
      case '.node':
        yield specifier
    }
  }

  function * resolveDirectory (specifier) {
    const pkg = path.join(specifier, 'package.json')

    let info
    try {
      info = Module.load(pkg).exports
    } catch {
      info = null
    }

    if (info) {
      if (exports.path) {
        const name = info.name.replace(/\//g, '+')
        const version = info.version

        for (const candidate of [
          `${name}.bare`,
          `${name}@${version}.bare`,
          `${name}.node`,
          `${name}@${version}.node`
        ]) {
          const file = path.join(exports.path, candidate)

          try {
            const protocol = Module._protocols['file:']

            if (protocol.exists(file)) yield file
          } catch {}
        }
      }

      try {
        specifier = path.dirname(Module.resolve(path.join(info.name, 'package.json')))
      } catch {}
    }

    for (const directory of resolveAddonPaths(specifier)) {
      try {
        const files = bare.readdir(directory)

        for (const file of files) {
          yield * resolveFile(path.join(directory, file))
        }
      } catch {}
    }
  }

  function * resolveAddonPaths (specifier) {
    yield path.join(specifier, 'build/Debug')
    yield path.join(specifier, 'build/Release')
    yield path.join(specifier, 'build')
    yield path.join(specifier, 'prebuilds', `${process.platform}-${process.arch}`)
  }
}
