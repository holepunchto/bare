import assert from 'assert'

const { default: mod } = await import('./fixtures/esm')

assert(mod === 'Hello from ESM')
