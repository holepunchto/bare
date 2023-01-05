const timers = require('tiny-timers-native')

console.log('Setting timer...')
timers.setTimeout(function () {
  console.log('I ran after 1s!')
}, 1000)
