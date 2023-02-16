const Module = require('module')
const path = require('path')

Module.load(
  process.argv[1] = Module.resolve(
    path.resolve(process.cwd(), process.argv[1])
  )
)
