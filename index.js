const { Readable, Writable } = require('streamx')

let tick = 0
let wrote = 0

const rs = new Readable({
  read (cb) {
    rs.push('#' + (++tick))
    if (tick === 20) rs.push(null)
    cb(null)
  }
})

rs.on('data', function (data) {
  console.log('ondata:', data)
})

// const ws = new Writable({
//   write (data, cb) {
//     wrote++
//     cb(null)
//   }
// })
//
// rs.pipe(ws).on('finish', function () {
//   console.log('done', wrote)
// })
