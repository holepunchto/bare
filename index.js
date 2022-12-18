const console = {
  log (...msg) {
    let s = ''
    for (const m of msg) s += m + ' '
    process._print(s.trim() + '\n')
  }
}

function onfsopen (id, fd) {
  console.log('callback to js', id, fd)
}

const fs = loadAddon('./addons/fs.pear', 'bootstrap_fs')

const req = new Uint8Array(fs.size_of_pearfs_req_t)
const name = './index.js'

fs.init(onfsopen)
fs.open(req, name)

function loadAddon (name, fn) {
  return process._loadAddon(name, fn)
}
