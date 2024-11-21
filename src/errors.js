exports.AddonError = class AddonError extends Error {
  constructor(msg, code, fn = AddonError) {
    super(`${code}: ${msg}`)
    this.code = code

    if (Error.captureStackTrace) {
      Error.captureStackTrace(this, fn)
    }
  }

  get name() {
    return 'AddonError'
  }

  static ADDON_NOT_FOUND(msg) {
    return new AddonError(msg, 'ADDON_NOT_FOUND', AddonError.ADDON_NOT_FOUND)
  }

  static UNSUPPORTED_PROTOCOL(msg) {
    return new AddonError(
      msg,
      'UNSUPPORTED_PROTOCOL',
      AddonError.UNSUPPORTED_PROTOCOL
    )
  }
}
