/* global bare */

const Module = require('bare-module')
const path = require('bare-path')
const { fileURLToPath, pathToFileURL } = require('bare-url')
const { ProtocolError } = require('./errors')

module.exports = new Module.Protocol({
  postresolve(url) {
    switch (url.protocol) {
      case 'file:':
        try {
          return pathToFileURL(bare.realpath(path.toNamespacedPath(fileURLToPath(url))))
        } catch (err) {
          throw ProtocolError.CANNOT_RESOLVE(`Cannot resolve module '${url.href}'`, url, err)
        }
      default:
        return url
    }
  },

  exists(url, type = 0) {
    switch (url.protocol) {
      case 'file:':
        return bare.exists(
          path.toNamespacedPath(fileURLToPath(url)),
          type === Module.constants.types.ASSET ? bare.FILE | bare.DIR : bare.FILE
        )
      default:
        return false
    }
  },

  read(url) {
    switch (url.protocol) {
      case 'file:':
        try {
          return Buffer.from(bare.read(path.toNamespacedPath(fileURLToPath(url))))
        } catch (err) {
          throw ProtocolError.CANNOT_READ(`Cannot read module '${url.href}'`, url, err)
        }
      default:
        throw ProtocolError.UNKNOWN_PROTOCOL(
          `Unknown protocol '${url.protocol}' for module '${url.href}'`,
          url
        )
    }
  }
})
