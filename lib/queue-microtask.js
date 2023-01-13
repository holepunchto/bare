const resolved = Promise.resolve()
module.exports = function queueMicrotask (fn) {
  resolved.then(fn)
}
