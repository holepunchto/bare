/* global bare */

module.exports = exports = function addon (specifier) {
  if (typeof specifier !== 'string') throw new TypeError('dirname must be a string')

  if (exports.cache[specifier]) return exports.cache[specifier]

  const addon = exports.cache[specifier] = bare.loadAddon(specifier)

  return addon
}

exports.cache = Object.create(null)

exports.resolve = function resolve (specifier) {
  if (typeof specifier !== 'string') throw new TypeError('dirname must be a string')

  return bare.resolveAddon(specifier)
}
