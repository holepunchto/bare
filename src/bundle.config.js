module.exports = {
  protocol: 'bare',
  format: 'js',
  target: 'c',
  name: 'bare_bundle',
  builtins: [],
  importMap: require.resolve('./import-map.json'),
  header: '(function (bare) {',
  footer: '})'
}
