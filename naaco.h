#ifndef SRC_NAACO_H_
#define SRC_NAACO_H_

#if __cplusplus >= 202002L

#include <napi.h>
#include <coroutine>
#include <variant>

#ifdef NAPI_CPP_EXCEPTIONS

#define NAPI_CO_THROW(e, ...) throw e
#define NAPI_CO_THROW_VOID(e) throw e

#define NAPI_CO_THROW_IF_FAILED(env, status, ...)                              \
  if ((status) != napi_ok) throw Napi::Error::New(env);

#define NAPI_CO_THROW_IF_FAILED_VOID(env, status)                              \
  if ((status) != napi_ok) throw Napi::Error::New(env);

#else

#define NAPI_CO_THROW(e, ...)                                                  \
  do {                                                                         \
    (e).ThrowAsJavaScriptException();                                          \
    co_return __VA_ARGS__;                                                     \
  } while (0)

#define NAPI_CO_THROW_VOID(e)                                                  \
  do {                                                                         \
    (e).ThrowAsJavaScriptException();                                          \
    co_return;                                                                 \
  } while (0)

#define NAPI_CO_THROW_IF_FAILED(env, status, ...)                              \
  if ((status) != napi_ok) {                                                   \
    Napi::Error::New(env).ThrowAsJavaScriptException();                        \
    co_return __VA_ARGS__;                                                     \
  }

#define NAPI_CO_THROW_IF_FAILED_VOID(env, status)                              \
  if ((status) != napi_ok) {                                                   \
    Napi::Error::New(env).ThrowAsJavaScriptException();                        \
    co_return;                                                                 \
  }

#endif

namespace naaco {

class CoPromise : public Napi::Promise {
 public:
  class promise_type;
  class Awaiter;

  CoPromise(napi_env env, napi_value value): Napi::Promise(env, value) {}
};

class CoPromise::promise_type {
 public:
  template <typename... Args>
  promise_type(napi_env env, Args&&...);

  template <typename... Args>
  promise_type(Napi::Value value, Args&&...);

  promise_type(const Napi::CallbackInfo& info);

  naaco::CoPromise get_return_object() const;
  std::suspend_never initial_suspend() const NAPI_NOEXCEPT;
  std::suspend_never final_suspend() const NAPI_NOEXCEPT;
  void unhandled_exception() const;
  void return_value(napi_value value) const;
  Awaiter await_transform(Napi::Value value) const NAPI_NOEXCEPT;

  void Resolve(napi_value value) const;
  void Reject(napi_value value) const;

 private:
  Napi::Env env_;
  Deferred deferred_;
};

class CoPromise::Awaiter {
 public:
  Awaiter(Napi::Value value);

  bool await_ready() const NAPI_NOEXCEPT;
  void await_suspend(std::coroutine_handle<> handle);
  Napi::Value await_resume() const;

 protected:
  std::coroutine_handle<> handle_;
  std::variant<Napi::Value, Napi::Value, Napi::Value> state_;

  virtual void Fulfill(Napi::Value value);
  virtual void Reject(Napi::Value reason);

  static Napi::Value OnFulfill(const Napi::CallbackInfo&);
  static Napi::Value OnReject(const Napi::CallbackInfo&);
};

}  // namespace naaco

#include "naaco-inl.h"

#else
#error "C++20 is required to use coroutines."
#endif

#endif
