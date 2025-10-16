exports.AddonError = class AddonError extends Error {
  constructor(msg, fn = AddonError, code = fn.name) {
    super(`${code}: ${msg}`)
    this.code = code

    if (Error.captureStackTrace) {
      Error.captureStackTrace(this, fn)
    }
  }

  get name() {
    return 'AddonError'
  }

  static ADDON_NOT_FOUND(msg, specifier, referrer = null, candidates = []) {
    const err = new AddonError(msg, AddonError.ADDON_NOT_FOUND)

    err.specifier = specifier
    err.referrer = referrer
    err.candidates = candidates

    return err
  }

  static UNSUPPORTED_PROTOCOL(msg) {
    return new AddonError(msg, AddonError.UNSUPPORTED_PROTOCOL)
  }
}
