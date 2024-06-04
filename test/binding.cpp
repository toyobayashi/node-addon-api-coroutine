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
  exports.Set("coroutineFunction", Napi::Function::New(env, CoroutineFunction));
  return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
