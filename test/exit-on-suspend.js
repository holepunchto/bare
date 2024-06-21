/* global Bare */
Bare
  .on('suspend', () => {
    Bare.exit()
  })
  .suspend()
