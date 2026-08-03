exports.AddonError = class AddonError extends Error {
  constructor(msg, fn = AddonError, code = fn.name, opts = {}) {
    if (typeof code === 'object' && code !== null) {
      opts = code
      code = fn.name
    }

    super(`${code}: ${msg}`, opts)

    this.code = code

    if (Error.captureStackTrace) Error.captureStackTrace(this, fn)
  }

  get name() {
    return 'AddonError'
  }

  static ADDON_NOT_FOUND(msg, specifier, referrer = null, candidates = [], cause) {
    const err = new AddonError(msg, AddonError.ADDON_NOT_FOUND, cause ? { cause } : {})

    err.specifier = specifier
    err.referrer = referrer
    err.candidates = candidates

    return err
  }

  static CANNOT_LOAD(msg, url, cause) {
    const err = new AddonError(msg, AddonError.CANNOT_LOAD, cause ? { cause } : {})

    err.url = url

    return err
  }

  static UNKNOWN_PROTOCOL(msg, url) {
    const err = new AddonError(msg, AddonError.UNKNOWN_PROTOCOL)

    err.url = url

    return err
  }
}

exports.ProtocolError = class ProtocolError extends Error {
  constructor(msg, fn = ProtocolError, code = fn.name, opts = {}) {
    if (typeof code === 'object' && code !== null) {
      opts = code
      code = fn.name
    }

    super(`${code}: ${msg}`, opts)

    this.code = code

    if (Error.captureStackTrace) Error.captureStackTrace(this, fn)
  }

  get name() {
    return 'ProtocolError'
  }

  static CANNOT_RESOLVE(msg, url, cause) {
    const err = new ProtocolError(msg, ProtocolError.CANNOT_RESOLVE, cause ? { cause } : {})

    err.url = url

    return err
  }

  static CANNOT_READ(msg, url, cause) {
    const err = new ProtocolError(msg, ProtocolError.CANNOT_READ, cause ? { cause } : {})

    err.url = url

    return err
  }

  static UNKNOWN_PROTOCOL(msg, url) {
    const err = new ProtocolError(msg, ProtocolError.UNKNOWN_PROTOCOL)

    err.url = url

    return err
  }
}
