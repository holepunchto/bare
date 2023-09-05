module.exports = {
  protocol: 'bare',
  format: 'js',
  target: 'c',
  name: 'bare_bundle',
  builtins: [],
  imports: {
    addon: '/src/builtins/addon.js',
    assert: '/src/builtins/assert.js',
    buffer: '/src/builtins/buffer.js',
    console: '/src/builtins/console.js',
    events: '/src/builtins/events.js',
    module: '/src/builtins/module.js',
    os: '/src/builtins/os.js',
    path: '/src/builtins/path.js',
    process: '/src/builtins/process.js',
    timers: '/src/builtins/timers.js',
    url: '/src/builtins/url.js'
  },
  header: '(function (bare) {',
  footer: '})',
  compress: ['*']
}
