# node-addon-api-coroutine

# Quick Start

```bash
npm install node-addon-api node-addon-api-coroutine
```

Create `binding.gyp`

```gyp
{
  "targets": [
    {
      'target_name': 'binding',
      'dependencies': [
        "<!(node -p \"require('node-addon-api').targets\"):node_addon_api_except",
        "<!(node -p \"require('node-addon-api-coroutine').targets\"):node_addon_api_coroutine"
      ],
      'sources': ['binding.cpp'],
    }
  ]
}
```

Create `binding.cpp`

```cpp
#include <naaco.h>

naaco::CoPromise CoroutineFunction(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::Value async_function = info[0];
  if (!async_function.IsFunction()) {
    NAPI_CO_THROW(Napi::Error::New(env, "not async function"), Napi::Value());
  }
  Napi::Value result = co_await async_function.As<Napi::Function>()({});
  co_return result;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("coroutineFunction", Function::New(env, CoroutineFunction));
  return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
```

Create `binding.js`

```js
const { coroutineFunction } = require('./build/Release/binding.node')

coroutineFunction(function () {
  return new Promise((resolve) => {
    setTimeout(() => {
      resolve(42)
    }, 1000)
  })
}).then(result => {
  console.log(result) // 42
})
```
