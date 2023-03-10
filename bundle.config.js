module.exports = {
  protocol: 'pear',
  format: 'js',
  target: 'c',
  name: 'pear_bundle',
  importMap: require.resolve('./src/import-map.json'),
  header: '(function (pear) {',
  footer: '})'
}
