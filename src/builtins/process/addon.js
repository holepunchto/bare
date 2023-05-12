/* global bare */

const path = require('../path')

module.exports = exports = function addon (specifier) {
  if (typeof specifier !== 'string') throw new TypeError('dirname must be a string')

  specifier = normalize(specifier)

  if (exports.cache[specifier]) return exports.cache[specifier]

  const addon = exports.cache[specifier] = bare.loadAddon(specifier)

  return addon
}

exports.cache = Object.create(null)

exports.resolve = function resolve (specifier) {
  if (typeof specifier !== 'string') throw new TypeError('dirname must be a string')

  specifier = normalize(specifier)

  return bare.resolveAddon(specifier)
}

function normalize (specifier) {
  const i = specifier.indexOf(path.join('/', 'node_modules'))

  if (i >= 0) specifier = path.join(process.cwd(), specifier.slice(i))

  return specifier
}
