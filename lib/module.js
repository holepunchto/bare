/* global pear */

const path = require('tiny-paths')
const events = require('@pearjs/events')
const timers = require('tiny-timers-native')

module.exports = class Module {
  constructor (filename, dirname) {
    this.filename = filename
    this.dirname = dirname
    this.exports = {}
  }

  _loadJSON () {
    this.exports = JSON.parse(pear.readSourceSync(this.filename))
    return this.exports
  }

  _loadJS () {
    const dirname = this.dirname

    require.cache = Module.cache
    require.resolve = resolve

    Module.runScript(this, pear.readSourceSync(this.filename), require)

    return this.exports

    function resolve (req) {
      return Module.resolve(req, dirname)
    }

    function require (req) {
      if (req === 'module') return Module
      if (req === 'path') return path
      if (req === 'events') return events
      if (req === 'timers') return timers
      if (req.endsWith('.node') || req.endsWith('.pear')) return pear.loadAddon(req, pear.ADDONS_DYNAMIC)
      const filename = resolve(req)
      return Module.load(filename)
    }
  }

  static cache = Object.create(null)

  static bootstrap (filename, process) {
    let mod = Module.cache[filename]

    if (mod) {
      process.main = mod
    }

    process.main = Module.cache[filename] = mod = new Module(filename, path.dirname(filename))

    if (filename.endsWith('.json')) mod._loadJSON()
    else mod._loadJS()
  }

  static load (filename) {
    if (Module.cache[filename]) return Module.cache[filename].exports

    const mod = new Module(filename, path.dirname(filename))

    Module.cache[filename] = mod

    return filename.endsWith('.json') ? mod._loadJSON() : mod._loadJS()
  }

  static runScript (module, source, require) {
    new Function('__dirname', '__filename', 'module', 'exports', 'require', source + '\n//# sourceURL=' + module.filename)( // eslint-disable-line
      module.dirname,
      module.filename,
      module,
      module.exports,
      require
    )
  }

  // TODO: align with 99% of https://nodejs.org/dist/latest-v18.x/docs/api/modules.html#all-together

  static resolve (req, dirname) {
    if (req.length === 0) throw new Error('Could not resolve ' + req + ' from ' + dirname)

    let p = null

    if (req[0] !== '.' && req[0] !== '/') {
      const target = 'node_modules/' + req

      while (true) {
        const nm = path.join(dirname, target)
        const type = pear.existsSync(nm)

        if (type === 0) {
          const parent = path.dirname(dirname)
          if (parent === nm) throw new Error('Could not resolve ' + req + ' from ' + dirname)
          dirname = parent
          continue
        }

        dirname = nm
        req = '.'
        break
      }
    }

    p = path.join(dirname, req)

    if (/\.(js|mjs|cjs|json)$/i.test(req) && pear.existsSync(p) === pear.FS_FILE) {
      return p
    }

    if (pear.existsSync(p + '.js') === pear.FS_FILE) {
      return p + '.js'
    }

    if (pear.existsSync(p + '.cjs') === pear.FS_FILE) {
      return p + '.cjs'
    }

    if (pear.existsSync(p + '.mjs') === pear.FS_FILE) {
      return p + '.mjs'
    }

    if (pear.existsSync(p + '.json') === pear.FS_FILE) {
      return p + '.json'
    }

    const pkg = path.join(p, 'package.json')

    if (pear.existsSync(pkg) === pear.FS_FILE) {
      const json = Module.load(pkg)
      p = path.join(p, json.main || 'index.js')

      if (pear.existsSync(p) !== pear.FS_FILE) {
        throw new Error('Could not resolve ' + req + ' from ' + dirname)
      }

      return p
    }

    p = path.join(p, 'index.js')
    if (pear.existsSync(p) === pear.FS_FILE) {
      return p
    }

    throw new Error('Could not resolve ' + req + ' from ' + dirname)
  }
}
