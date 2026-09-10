// An asset may name a directory, which the protocol enumerates through `list()`
// so that everything under it is reached.

const path = require('bare-path')
const traverse = require('bare-module-traverse')
const { pathToFileURL } = require('bare-url')
const t = require('bare-tap')

const { protocol } = module

t.plan(6)

t.ok(require.asset('./fixtures/assets/solo.txt').endsWith('solo.txt'))
t.ok(require.asset('./fixtures/assets/dir').endsWith('dir'))
t.ok(require.asset('./fixtures/assets/').endsWith('assets'))
t.ok(require.asset('.').endsWith('test'))

t.ok(protocol.listSync(pathToFileURL(__dirname)).resolved, 'a listing is resolved')

const entry = pathToFileURL(path.join(__dirname, 'fixtures/assets/entry.js'))

const found = []

for (const dependency of traverse(
  entry,
  { resolve: traverse.resolve.bare },
  readModule,
  listPrefix
)) {
  const { href } = dependency.url

  if (href.endsWith('.txt')) found.push(href.slice(href.indexOf('/assets/')))
}

t.equal(found.sort().join(), '/assets/dir/a.txt,/assets/dir/nested/b.txt,/assets/solo.txt')

function readModule(url) {
  return protocol.existsSync(url) ? protocol.readSync(url) : null
}

function listPrefix(url) {
  return protocol.listSync(url)
}
