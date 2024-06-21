/* global Bare */
Bare
  .on('idle', () => {
    Bare.exit()
  })
  .suspend()
