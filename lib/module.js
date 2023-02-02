/* global pear */

const path = require('@pearjs/path')
const events = require('@pearjs/events')
const timers = require('@pearjs/timers')

module.exports = class Module {
  constructor (filename, dirname = path.dirname(filename)) {
    this.filename = filename
    this.dirname = dirname
    this.exports = {}
  }

  _readSource () {
    return pear.readSourceSync(this.filename)
  }

  _loadJSON (source = this._readSource()) {
    this.exports = JSON.parse(source)
    return this.exports
  }

  _loadJS (source = this._readSource()) {
    const dirname = this.dirname

    require.cache = Module.cache
    require.resolve = resolve

    Module.runScript(this, source, require)

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

  static bootstrap (filename, source) {
    const mod = Module.cache[filename] = new Module(filename)

    filename.endsWith('.json')
      ? mod._loadJSON(source)
      : mod._loadJS(source)
  }

  static load (filename) {
    if (Module.cache[filename]) return Module.cache[filename].exports

    const mod = Module.cache[filename] = new Module(filename)

    return filename.endsWith('.json')
      ? mod._loadJSON()
      : mod._loadJS()
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
    let ticks = 16386 // tons of allowed ticks, just so loops break if a user makes one...

    if (req[0] !== '.' && req[0] !== '/') {
      const [name, rest] = splitModule(req)
      const target = 'node_modules/' + name

      let tmp = dirname

      while (ticks-- > 0) {
        const nm = path.join(tmp, target)
        const type = pear.existsSync(nm)

        if (type === 0) {
          const parent = path.dirname(tmp)
          if (parent === tmp) ticks = -1
          else tmp = parent
          continue
        }

        dirname = nm
        req = rest
        break
      }
    }

    while (ticks-- > 0) {
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

        dirname = p
        req = json.main || 'index.js'
        continue
      }

      p = path.join(p, 'index.js')
      if (pear.existsSync(p) === pear.FS_FILE) {
        return p
      }

      break
    }

    throw new Error('Could not resolve ' + req + ' from ' + dirname)
  }
}

function splitModule (m) {
  const i = m.indexOf('/')
  if (i === -1) return [m, '.']

  return [m.slice(0, i), '.' + m.slice(i)]
}
