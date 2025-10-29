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
    const err = new AddonError(msg, AddonError.ADDON_NOT_FOUND, { cause })

    err.specifier = specifier
    err.referrer = referrer
    err.candidates = candidates

    return err
  }

  static UNSUPPORTED_PROTOCOL(msg) {
    return new AddonError(msg, AddonError.UNSUPPORTED_PROTOCOL)
  }
}
