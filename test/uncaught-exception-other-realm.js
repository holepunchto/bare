// Errors and rejections from another realm reach the listeners as placeholders
// from this one. Reading the originals would hand their getters arguments from
// this realm, which the getters below use to reach `Bare` and exit with 42.

const t = require('bare-tap')
const Realm = require('bare-realm')

t.plan(4)

const realm = new Realm()

const hostile = realm.evaluate(`
  const escape = new Proxy(function () {}, {
    apply (target, self, args) {
      if (!(args instanceof Array)) args.constructor.constructor('return Bare')().exit(42)
    }
  })

  globalThis.hostile = {}

  for (const key of ['stack', 'message', 'name', 'cause', 'code']) {
    Object.defineProperty(hostile, key, { get: escape })
  }

  hostile
`)

Bare.on('uncaughtException', (err) => {
  t.equal(err.message, 'Uncaught exception in another realm')
})

Bare.on('unhandledRejection', (reason, promise) => {
  t.equal(reason.message, 'Unhandled rejection in another realm')
  t.equal(promise, undefined)
})

setTimeout(() => {
  t.pass()
}, 0)

queueMicrotask(() => {
  throw hostile
})

realm.evaluate('Promise.reject(hostile)')
