module.exports = {
  header:
`(function (bare) {
  const require = () => {
    throw new Error('Cannot require modules from bootstrap script')
  }

  require.addon = (specifier) => {
    return bare.addon(specifier)
  }
`,
  footer: '})'
}
