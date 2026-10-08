const fs = require('fs')
const path = require('path')

fs.copyFileSync(path.join(__dirname, '..', 'src', 'bare.d.ts'), path.join(__dirname, 'index.d.ts'))
