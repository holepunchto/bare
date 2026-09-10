/* global bare */

const Module = require('bare-module')
const path = require('bare-path')
const { fileURLToPath, pathToFileURL } = require('bare-url')
const { ProtocolError } = require('./errors')

module.exports = new Module.Protocol({
  resolve(url) {
    if (url.protocol !== 'file:') return url

    try {
      return pathToFileURL(bare.realpath(path.toNamespacedPath(fileURLToPath(url))))
    } catch (err) {
      throw ProtocolError.CANNOT_RESOLVE(`Cannot resolve module '${url.href}'`, url, err)
    }
  },

  exists(url) {
    if (url.protocol !== 'file:') return false

    try {
      return bare.exists(path.toNamespacedPath(fileURLToPath(url)))
    } catch {
      return false
    }
  },

  read(url) {
    if (url.protocol !== 'file:') return null

    try {
      return Buffer.from(bare.read(path.toNamespacedPath(fileURLToPath(url))))
    } catch (err) {
      throw ProtocolError.CANNOT_READ(`Cannot read module '${url.href}'`, url, err)
    }
  },

  list(url) {
    const listing = list(this, url)

    listing.resolved = true

    return listing
  }
})

function* list(protocol, url) {
  if (url.protocol !== 'file:') return

  let resolution

  try {
    resolution = protocol.resolve(url)
  } catch {
    return
  }

  if (protocol.exists(resolution)) return yield resolution

  yield* listDirectory(protocol, fileURLToPath(resolution))
}

function* listDirectory(protocol, dirname) {
  let entries

  try {
    entries = bare.list(path.toNamespacedPath(dirname))
  } catch {
    return
  }

  const { names, types } = entries

  for (let i = 0, n = names.length; i < n; i++) {
    const entry = path.join(dirname, names[i])

    switch (types[i]) {
      case bare.DIRENT_FILE:
        yield pathToFileURL(entry)
        break
      case bare.DIRENT_DIR:
        yield* listDirectory(protocol, entry)
        break
      default:
        yield* list(protocol, pathToFileURL(entry))
    }
  }
}
