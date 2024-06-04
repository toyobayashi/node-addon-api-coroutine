{
  'targets': [
    {
      'target_name': 'node_addon_api_coroutine',
      'type': 'none',
      'sources': [ 'naaco.h', 'naaco-inl.h' ],
      'direct_dependent_settings': {
        'include_dirs': [ '.' ],
        'includes': ['naaco.gypi'],
      }
    }
  ]
}
