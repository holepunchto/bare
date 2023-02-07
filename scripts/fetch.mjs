import path from 'path'
import url from 'url'
import fs from 'fs/promises'
import Hyperdrive from 'hyperdrive'
import Localdrive from 'localdrive'
import Corestore from 'corestore'
import Hyperswarm from 'hyperswarm'
import id from 'hypercore-id-encoding'

const __filename = url.fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)

const store = new Corestore(path.join(__dirname, '../corestore'))
const drive = new Hyperdrive(store, id.decode('dphphcdt16t4igfutyn9wikcn69trsd5qamgjwhxyjyfa4mix4fo'))
const swarm = new Hyperswarm()

const host = process.platform + '-' + process.arch
const prebuilds = path.join(__dirname, '../prebuilds')

swarm.on('connection', c => store.replicate(c))

await drive.ready()
const done = store.findingPeers()

swarm.join(drive.discoveryKey)
swarm.flush().then(done)

for await (const diff of drive.mirror(new Localdrive(prebuilds), { prefix: '/' + host })) {
  console.log(diff.op, diff.key, `+${diff.bytesAdded}`, `-${diff.bytesRemoved}`)
}

await fs.rm(path.join(prebuilds, 'host'), { force: true, recursive: true })

await fs.symlink(host, path.join(prebuilds, 'host'))

await swarm.destroy()
