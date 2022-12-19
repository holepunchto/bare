const { Readable, Writable } = require('streamx')

let tick = 0
let wrote = 0

const rs = new Readable({
  read (cb) {
    rs.push('#' + (++tick))
    if (tick === 1e6) rs.push(null)
    cb(null)
  }
})

const ws = new Writable({
  write (data, cb) {
    wrote++
    cb(null)
  }
})

console.time()
rs.pipe(ws).on('close', function () {
  console.timeEnd()
  console.log('done', wrote)
})
