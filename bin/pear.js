const Module = require('module')
const path = require('path')

const argv = require('minimist')(process.argv.slice(1), {
  stopEarly: true,
  boolean: [
    'version'
  ],
  alias: {
    version: 'v'
  }
})

process.argv = [process.argv[0], ...argv._]

if (argv.version) {
  const pkg = require('../package.json')

  console.log(`v${pkg.version}`)

  process.exit()
}

Module.load(
  process.argv[1] = Module.resolve(
    path.resolve(process.cwd(), process.argv[1])
  )
)
