// console.log(process._loadAddon)

const addon = process._loadAddon('./node_modules/tiny-timers-native/build/Release/tiny_timers.node')

for (const k of Object.keys(addon)) {
  console.log('key', k)
}
// console.log()

// const { Readable, Writable } = require('streamx')

// console.time()
// for (let i = 0; i < 1e7; i++) {
//   const t = process.hrtime()
// }
// console.timeEnd()

// process.exit()

// let tick = 0
// let wrote = 0

// const rs = new Readable({
//   read (cb) {
//     rs.push('#' + (++tick))
//     if (tick === 1e7) rs.push(null)
//     cb(null)
//   }
// })

// const ws = new Writable({
//   write (data, cb) {
//     wrote++
//     cb(null)
//   }
// })

// console.time()
// rs.pipe(ws).on('close', function () {
//   console.timeEnd()
//   console.log('done', wrote)
// })
