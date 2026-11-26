const fs = require('bare-fs')
const os = require('bare-os')
const path = require('bare-path')
const url = require('bare-url')
const t = require('bare-tap')
const { Addon } = Bare

t.plan(2)

const temp = fs.mkdtempSync(path.join(os.tmpdir(), 'bare-'))

Bare.once('exit', () => fs.rmSync(temp, { recursive: true }))

// Exceeds the 260 character MAX_PATH limit on Windows.
const dirname = path.join(temp, 'a'.repeat(100), 'b'.repeat(100), 'c'.repeat(100))

fs.mkdirSync(dirname, { recursive: true })

const filename = path.join(dirname, 'addon.bare')

fs.copyFileSync(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`, filename)

const href = url.pathToFileURL(filename)

t.equal(new Addon(href).exports, 'Hello from addon')
t.equal(Addon.loaded(href), true)
