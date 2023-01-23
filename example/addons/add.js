const add = require('./build/add_numbers.pear')
const { addNumbersNativeSlow, addNumbersNativeFast } = add

for (let runs = 0; runs < 3; runs++) {
  console.log('run ' + runs)

  console.time('js')

  for (let i = 0; i < 1e8; i++) {
    addNumbersJS(i, i)
  }

  console.timeEnd('js')
  console.time('native-slow')

  for (let i = 0; i < 1e8; i++) {
    addNumbersNativeSlow(i, i)
  }

  console.timeEnd('native-slow')
  console.time('native-fast')

  for (let i = 0; i < 1e8; i++) {
    addNumbersNativeFast(i, i)
  }

  console.timeEnd('native-fast')
  console.log()
}

function addNumbersJS (a, b) {
  return a + b
}
