#include <iostream>
#include <naaco.h>
#include <list>

using namespace Napi;
using namespace naaco;

CoPromise NestedCoroutine(const CallbackInfo& info) {
  Env env = info.Env();
  Value async_function = info[0];
  if (!async_function.IsFunction()) {
    NAPI_CO_THROW(Error::New(env, "not function"), Value());
  }
  auto maybe_promise = async_function.As<Function>()({});
  Value result;
#ifdef NODE_ADDON_API_ENABLE_MAYBE
  if (maybe_promise.IsNothing()) {
    NAPI_CO_THROW(env.GetAndClearPendingException(), Value());
  } else {
    result = co_await maybe_promise.Unwrap();
  }
#else
  result = co_await maybe_promise;
#endif
  result = co_await result;
  co_return co_await Number::New(env, result.As<Number>().DoubleValue() * 2);
}

CoPromise Coroutine(const CallbackInfo& info) {
  Env env = info.Env();
  Value number = co_await NestedCoroutine(info);
  co_return Number::New(env, number.As<Number>().DoubleValue() * 2);
}

CoPromise CoroutineThrow(const CallbackInfo& info) {
  Env env = info.Env();
  co_await NestedCoroutine(info);
  NAPI_CO_THROW(Error::New(env, "test error"), Value());
  co_return Value();
}

CoPromise TestOrderInner(Array array) {
  Napi::Env env = array.Env();
  ObjectReference array_ref = ObjectReference::New(array, 1);
  array_ref.Set(array_ref.Value().As<Array>().Length(),
                Napi::Number::New(env, 1));
  co_await Napi::Number::New(env, 1);
  array_ref.Set(array_ref.Value().As<Array>().Length(),
                Napi::Number::New(env, 2));
  co_return env.Undefined();
}

CoPromise TestOrder(const CallbackInfo& info) {
  Napi::Env env = info.Env();
  Array array = Array::New(env);
  Value promise = TestOrderInner(array);
  array.Set(array.Length(), Napi::Number::New(env, 3));
  ObjectReference array_ref = ObjectReference::New(array, 1);
  co_await promise;
  co_return array_ref.Value();
}

struct Task {
  using CompleteCb = std::function<void(String)>;

  struct Awaiter;
  struct promise_type {
    String last_error;
    std::list<CompleteCb> list_;
    napi_env env_;
    Napi::Promise::Deferred deferred_;
    promise_type (napi_env env): 
      last_error(), list_(), env_(env), deferred_(Napi::Promise::Deferred::New(env)) {}

    ~promise_type() {
      std::for_each(list_.cbegin(), list_.cend(), [this](CompleteCb f) {
        f(last_error);
      });
    }

    Task get_return_object() {
      return {std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
    void return_value(std::tuple<Napi::Value, Napi::Value> result) {
      Napi::HandleScope scope(env_);
      Napi::Env env = Napi::Env(env_);
      Napi::Array arr = Napi::Array::New(env, 2);
      arr.Set(0u, std::get<0>(result));
      arr.Set(1u, std::get<1>(result));
      deferred_.Resolve(arr);
    }

    Awaiter await_transform(Value value) { return {value}; }

    void OnComplete(CompleteCb f) { list_.push_back(f); }
  };

  struct Awaiter : public CoPromise::Awaiter {
    Awaiter(Value value) : CoPromise::Awaiter(value) {}

    void Reject(Value reason) override {
      std::coroutine_handle<promise_type>::from_address(handle_.address())
          .promise()
          .last_error = reason.As<String>();
      handle_.resume();
    }

    std::tuple<Value, Value> await_resume() const {
      const Value* ok = std::get_if<1>(&state_);
      if (ok) return std::tuple<Value, Value>{*ok, ok->Env().Undefined()};
      const Value* err = std::get_if<2>(&state_);
      return std::tuple<Value, Value>{err->Env().Undefined(), *err};
    }
  };

  operator napi_value() const {
    return handle_.promise().deferred_.Promise();
  }

  Task(std::coroutine_handle<promise_type> handle) : handle_(handle) {}

  std::coroutine_handle<promise_type> handle_;

  void OnComplete(CompleteCb f) { handle_.promise().OnComplete(f); }
};

Task CreateTask(napi_env env) {
  Promise::Deferred deferred = Promise::Deferred::New(env);
  deferred.Reject(Napi::String::New(env, "task error"));
  std::tuple<Value, Value> result = co_await deferred.Promise();
  co_return result;
}

CoPromise OtherTypeCoroutine(const CallbackInfo& info) {
  Napi::Env env = info.Env();
  Task task = CreateTask(env);
  task.OnComplete([](String last_error) {
    if (!last_error.IsEmpty()) {
      std::cerr << last_error.Utf8Value() << std::endl;
    }
  });
  co_return task;
}

Object Init(Env env, Object exports) {
  exports.Set("coroutine", Function::New(env, Coroutine));
  exports.Set("coroutineThrow", Function::New(env, CoroutineThrow));
  exports.Set("testOrder", Function::New(env, TestOrder));
  exports.Set("otherTypeCoroutine", Function::New(env, OtherTypeCoroutine));
  return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
