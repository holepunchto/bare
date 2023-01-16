// loads a static native add on
module.exports = function (dir) {
  return pear.loadAddon(dir, pear.ADDONS_STATIC | pear.ADDONS_RESOLVE)
}
