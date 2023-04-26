module.exports = {
  protocol: 'pear',
  format: 'js',
  target: 'c',
  name: 'pear_bundle',
  importMap: require.resolve('./import-map.json'),
  header: '(function (pear) {',
  footer: '})'
}
