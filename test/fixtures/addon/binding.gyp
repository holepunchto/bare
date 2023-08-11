{
  'targets': [{
    'target_name': 'addon',
    'include_dirs': [
      '<!(bare-dev paths compat/napi)',
    ],
    'sources': [
      './binding.c',
    ],
    'configurations': {
      'Debug': {
        'defines': ['DEBUG'],
      },
      'Release': {
        'defines': ['NDEBUG'],
      },
    },
  }],
}
