const test = require('brittle')

test('basic', function (t) {
  t.plan(1)

  const start = Date.now()

  setTimeout(() => {
    const elapsed = Date.now() - start
    const diff = Math.abs(200 - elapsed)

    t.ok(diff < 50, 'Difference should be +-50 ms')
  }, 200)
})

test('clear', function (t) {
  const id = setTimeout(() => t.fail('Should have been cancelled'), 200)

  clearTimeout(id)
})
