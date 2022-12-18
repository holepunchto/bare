const console = {
  log (...msg) {
    let s = 'log '
    for (const m of msg) s += m + ' '
    process._print(stringToBuffer(s.trim()))
  }
}

function onfsopen (id, fd) {
  console.log('callback to js', id, fd)
}

const fs = loadAddon('./addons/fs.pear', 'bootstrap_fs')

const req = new Uint8Array(fs.size_of_pearfs_req_t)
const name = stringToBuffer('./index.js')

fs.init(onfsopen)
fs.open(req, name)

function loadAddon (name, fn) {
  return process._loadAddon(stringToBuffer(name), stringToBuffer(fn))
}

function stringToBuffer (s) {
  s += ''

  const b = new Uint8Array(s.length + 1)

  for (let i = 0; i < s.length; i++) {
    b[i] = s.charCodeAt(i)
  }

  return b
}
