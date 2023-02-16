const Module = require('module')
const path = require('path')

const entry = Module.resolve(path.resolve(process.cwd(), process.argv[1]))

Module.load(process.argv[1] = entry)
