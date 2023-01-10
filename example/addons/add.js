const add = require('./build/add_numbers.pear')
const { addNumbersNativeSlow, addNumbersNativeFast } = add

for (let runs = 0; runs < 3; runs++) {
  console.log('run ' + runs)

  console.time('js')

  let n = 0
  for (let i = 0; i < 1e8; i++) {
    n = addNumbersJS(i, i)
  }

  console.timeEnd('js')
  console.time('native-slow')

  n = 0
  for (let i = 0; i < 1e8; i++) {
    n = addNumbersNativeSlow(i, i)
  }

  console.timeEnd('native-slow')
  console.time('native-fast')

  n = 0
  for (let i = 0; i < 1e8; i++) {
    n = addNumbersNativeFast(i, i)
  }

  console.timeEnd('native-fast')
  console.log()
}

function addNumbersJS (a, b) {
  return a + b
}
