const assert = require('bare-assert')
const { Thread } = Bare

const isSubtest = !Thread.isMainThread

let planned = 0
let assertions = 0

Bare.once('exit', () => {
  Bare.once('exit', () => assert(planned, assertions))
})

if (!isSubtest) console.log('TAP version 14')

function printPlan() {
  let result = ''

  if (isSubtest) result += '    '

  result += '1..' + planned

  console.log(result)
}

function print(ok, message) {
  let result = ''

  if (isSubtest) result += '    '

  result += ok ? 'ok' : 'not ok'
  result += ' ' + assertions

  if (message) result += ' - ' + message

  console.log(result)
}

exports.sleep = function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms))
}

exports.plan = function plan(n) {
  planned = n
  printPlan()
}

exports.ok = function ok(value, message) {
  assertions++
  assert.ok(value, message)
  print(true, message)
}

exports.notOk = function notOk(value, message) {
  assertions++
  assert.notOk(value, message)
  print(true, message)
}

exports.equal = function equal(actual, expected, message) {
  assertions++
  assert.equal(actual, expected, message)
  print(true, message)
}

exports.notEqual = function notEqual(actual, expected, message) {
  assertions++
  assert.notEqual(actual, expected, message)
  print(true, message)
}

exports.pass = function pass(message) {
  assertions++
  print(true, message)
}

exports.fail = function fail(message) {
  assert.fail(message)
}
