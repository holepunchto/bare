const path = require('bare-path')
const { spawnSync } = require('bare-subprocess')

// Runs a fixture that is expected to abort, reporting whether it did along with
// what it wrote to stderr.
module.exports = function abort(fixture) {
  const { status, signal, stderr } = spawnSync(Bare.argv[0], [
    path.join(__dirname, '..', 'fixtures', fixture)
  ])

  return {
    // On Windows, `abort()` terminates the process with a non-zero exit code
    // rather than a signal.
    aborted: Bare.platform === 'win32' ? status !== 0 : signal === 'SIGABRT',
    stderr: stderr.toString()
  }
}
