const path = require('bare-path')
const { spawnSync } = require('bare-subprocess')

// Runs a fixture that is expected to abort, reporting whether it did along with
// what it wrote to stderr.
module.exports = function abort(fixture) {
  const result = spawnSync(Bare.argv[0], [path.join(__dirname, '..', 'fixtures', fixture)])

  const stderr = result.stderr.toString()

  if (stderr) console.error(stderr.trimEnd())

  // On Windows, `abort()` terminates the process with a non-zero exit code
  // rather than a signal.
  const aborted =
    (Bare.platform === 'win32' ? result.status !== 0 : result.signal === 'SIGABRT') &&
    !stderr.includes('ERROR: AddressSanitizer')

  return { aborted, stderr }
}
