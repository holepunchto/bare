import Hyperdrive from 'hyperdrive'
import Localdrive from 'localdrive'
import Corestore from 'corestore'
import Hyperswarm from 'hyperswarm'
import id from 'hypercore-id-encoding'
import path from 'path'
import os from 'os'
import fs from 'fs'

const store = new Corestore(path.join(os.tmpdir(), 'corestore'))
const drive = new Hyperdrive(store, id.decode('dphphcdt16t4igfutyn9wikcn69trsd5qamgjwhxyjyfa4mix4fo'))
const swarm = new Hyperswarm()

const host = process.platform + '-' + process.arch
const prebuilds = path.join(import.meta.url.slice(7), '../../prebuilds/v8')

swarm.on('connection', c => store.replicate(c))

await drive.ready()
const done = store.findingPeers()

swarm.join(drive.discoveryKey)
swarm.flush().then(done)

for await (const diff of drive.mirror(new Localdrive(prebuilds), { prefix: '/' + host })) {
  console.log(diff)
}

try {
  await fs.promises.unlink(path.join(prebuilds, 'host'))
} catch {}

await fs.promises.symlink(path.join(prebuilds, host), path.join(prebuilds, 'host'))

await swarm.destroy()
