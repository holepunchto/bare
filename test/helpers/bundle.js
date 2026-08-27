const Bundle = require('bare-bundle')
const traverse = require('bare-module-traverse')
const { pathToFileURL } = require('bare-url')

const { protocol, imports, resolutions } = module

module.exports = function bundle(entry, callback = null) {
  if (typeof entry === 'string') entry = pathToFileURL(entry)

  const source = callback === null ? null : `(${callback.toString()})(Bare.Thread.self.data)`

  const bundle = new Bundle()

  for (const dependency of traverse(
    entry,
    { imports, resolutions, resolve: traverse.resolve.bare },
    readModule
  )) {
    bundle.write(dependency.url.href, dependency.source, {
      main: dependency.url.href === entry.href,
      imports: dependency.imports
    })
  }

  return bundle.toBuffer({ shared: true })

  function readModule(url) {
    if (source !== null && url.href === entry.href) return source

    if (protocol.exists(url)) return protocol.read(url)

    return null
  }
}
