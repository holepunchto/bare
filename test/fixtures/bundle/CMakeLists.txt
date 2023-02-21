add_pear_bundle(
  ENTRY mod.js
  OUT mod.bundle
)

add_custom_target(
  test_bundle
  ALL
  DEPENDS mod.bundle
)
