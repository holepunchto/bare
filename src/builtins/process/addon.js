/* global bare */

const path = require('path')

module.exports = exports = function addon (specifier) {
  if (exports.cache[specifier]) return exports.cache[specifier]

  let addon = null

  try {
    addon = bare.loadStaticAddon(specifier)
  } catch {}

  if (addon === null) {
    addon = bare.loadDynamicAddon(resolve(specifier))
  }

  return (exports.cache[specifier] = addon)
}

exports.cache = Object.create(null)

exports.path = null

const resolve = exports.resolve = function resolve (specifier) {
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
