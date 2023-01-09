const UDX = require('udx-native')

const u = new UDX()

for (const net of u.networkInterfaces()) {
  console.log(net)
}
