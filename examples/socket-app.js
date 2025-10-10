const assert = require('bare-assert')
const tcp = require('bare-tcp')

const socket = Bare.getInheritedLowFd(3)
assert(socket !== null, 'Failed to get inherited socket on fd 3')

const server = new tcp.Server()

const listener = new tcp.Socket({ fd: socket.fd })

listener.on('connection', (conn) => {
  conn.on('data', (buffer) => {
    const message = buffer.toString().trim()

    if (message === 'PING') {
      console.log('Socket app received PING, sending PONG...')
      conn.write('PONG\n')
      conn.end()
      Bare.exit()
    }
  })
})

console.log('Server started using inherited systemd socket.')
