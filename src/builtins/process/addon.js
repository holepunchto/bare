/* global pear */

const path = require('@pearjs/path')

module.exports = exports = function addon (specifier) {
  if (typeof specifier !== 'string') throw new TypeError('dirname must be a string')

  specifier = normalize(specifier)

  if (exports.cache[specifier]) return exports.cache[specifier]

  const addon = exports.cache[specifier] = pear.loadAddon(specifier)

  return addon
}

exports.cache = Object.create(null)

exports.resolve = function resolve (specifier) {
  if (typeof specifier !== 'string') throw new TypeError('dirname must be a string')

  specifier = normalize(specifier)

  return pear.resolveAddon(specifier)
}

function normalize (specifier) {
  const i = specifier.indexOf('/node_modules/')

  if (i >= 0) specifier = path.join(process.cwd(), specifier.slice(i))

  return specifier
}