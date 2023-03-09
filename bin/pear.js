const Module = require('module')
const path = require('path')

const argv = require('minimist')(process.argv.slice(1), {
  stopEarly: true,
  boolean: [
    'version'
  ],
  string: [
    'import-map'
  ],
  alias: {
    version: 'v',
    'import-map': 'm'
  }
})

process.argv.splice(1, process.argv.length - 1, ...argv._)

if (argv.v) {
  const pkg = require('../package.json')

  console.log(`v${pkg.version}`)

  process.exit()
}

if (argv.m) {
  const { exports: map } = Module.load(
    Module.resolve(path.resolve(process.cwd(), argv.m))
  )

  if (map && map.imports) {
    for (const [from, to] of Object.entries(map.imports)) {
      Module._imports[from] = to
    }
  }
}

Module.load(
  process.argv[1] = Module.resolve(
    path.resolve(process.cwd(), process.argv[1])
  )
)
