const url = require('bare-url')
const t = require('bare-tap')
const Module = require('bare-module')
const { Addon } = Bare

t.plan(1)

// Resolving an addon reads the package's `package.json` to determine its name
// and version. That read must not populate the global module cache, as doing so
// leaks resolution state across otherwise isolated module graphs.

const parentURL = url.pathToFileURL('./test/fixtures/addon/')

const packageURL = new URL('package.json', parentURL)

Addon.resolve('.', parentURL)

t.ok(
  Module.cache[packageURL.href] === undefined,
  'addon resolution does not populate the global module cache'
)
