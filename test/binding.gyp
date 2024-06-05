{
  'target_defaults': {
    'includes': ['./common.gypi'],
    'dependencies': [
      "<!(node -p \"require('..').targets\"):node_addon_api_coroutine"
    ],
    'variables': {
      'build_sources_coroutine': [
        'coroutine.cc'
      ],
    }
  },
  "targets": [
    {
      'target_name': 'binding_cpp20',
      'dependencies': ["<!(node -p \"require('node-addon-api').targets\"):node_addon_api_except"],
      'sources': ['>@(build_sources_coroutine)'],
    },
    {
      'target_name': 'binding_cpp20_noexcept',
      'dependencies': ["<!(node -p \"require('node-addon-api').targets\"):node_addon_api"],
      'sources': ['>@(build_sources_coroutine)'],
    },
    {
      'target_name': 'binding_cpp20_noexcept_maybe',
      'dependencies': ["<!(node -p \"require('node-addon-api').targets\"):node_addon_api_maybe"],
      'sources': ['>@(build_sources_coroutine)'],
    },
    {
      'target_name': 'binding_cpp20_custom_namespace',
      'dependencies': ["<!(node -p \"require('node-addon-api').targets\"):node_addon_api"],
      'sources': ['>@(build_sources_coroutine)'],
      'defines': ['NAPI_CPP_CUSTOM_NAMESPACE=cstm'],
    },
    {
      'target_name': 'binding',
      'dependencies': ["<!(node -p \"require('node-addon-api').targets\"):node_addon_api_except"],
      'sources': ['binding.cpp'],
    },
  ]
}
