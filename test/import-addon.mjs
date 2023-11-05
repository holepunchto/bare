import assert from 'assert'
import build from './helpers/build.json'

const { default: mod } = await import(build.output.bare_addon)

assert(mod === 'Hello from addon')
