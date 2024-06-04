#ifndef SRC_NAACO_INL_H_
#define SRC_NAACO_INL_H_

namespace naaco {

template <typename... Args>
inline CoPromise::promise_type::promise_type(napi_env env, Args&&...)
    : env_(env), deferred_(CoPromise::Deferred::New(env)) {}

template <typename... Args>
inline CoPromise::promise_type::promise_type(Napi::Value value, Args&&...)
    : CoPromise::promise_type::promise_type(value.Env()) {}

inline CoPromise::promise_type::promise_type(const Napi::CallbackInfo& info)
    : CoPromise::promise_type::promise_type(info.Env()) {}

inline CoPromise CoPromise::promise_type::get_return_object() const {
  return deferred_.Promise().As<CoPromise>();
}

inline std::suspend_never CoPromise::promise_type::initial_suspend() const
    NAPI_NOEXCEPT {
  return {};
}
inline std::suspend_never CoPromise::promise_type::final_suspend() const
    NAPI_NOEXCEPT {
  return {};
}

inline void CoPromise::promise_type::unhandled_exception() const {
  std::exception_ptr exception = std::current_exception();
#ifdef NAPI_CPP_EXCEPTIONS
  try {
    std::rethrow_exception(exception);
  } catch (const Napi::Error& e) {
    deferred_.Reject(e.Value());
  } catch (const std::exception& e) {
    deferred_.Reject(Napi::Error::New(env_, e.what()).Value());
  } catch (const Value& e) {
    deferred_.Reject(e);
  } catch (const std::string& e) {
    deferred_.Reject(Napi::Error::New(env_, e).Value());
  } catch (const char* e) {
    deferred_.Reject(Napi::Error::New(env_, e).Value());
  } catch (...) {
    deferred_.Reject(Napi::Error::New(env_, "Unknown Error").Value());
  }
#else
  std::rethrow_exception(exception);
#endif
}

inline void CoPromise::promise_type::return_value(napi_value value) const {
  if (env_.IsExceptionPending()) {
    Reject(env_.GetAndClearPendingException().Value());
  } else {
    Resolve(value);
  }
}

inline CoPromise::Awaiter CoPromise::promise_type::await_transform(Napi::Value value) const NAPI_NOEXCEPT {
  return CoPromise::Awaiter(value);
}

inline void CoPromise::promise_type::Resolve(napi_value value) const {
  deferred_.Resolve(value);
}

inline void CoPromise::promise_type::Reject(napi_value value) const {
  deferred_.Reject(value);
}

inline CoPromise::Awaiter::Awaiter(Value value)
#ifdef NAPI_CPP_EXCEPTIONS
    : enabled_exceptions_(true),
#else
    : enabled_exceptions_(false),
#endif
      handle_(),
      state_(std::in_place_index<0>, value) {
}

inline bool CoPromise::Awaiter::await_ready() const NAPI_NOEXCEPT {
  return false;
}

inline void CoPromise::Awaiter::await_suspend(std::coroutine_handle<> handle) {
  handle_ = handle;
  const Napi::Value* value = std::get_if<0>(&state_);
  Napi::Env env = value->Env();
  Napi::Value promise;
  if (value->IsPromise()) {
    promise = value->As<Napi::Promise>();
#ifdef NODE_ADDON_API_ENABLE_MAYBE
  } else if (value->IsObject() &&
             value->As<Napi::Object>().Get("then").UnwrapOr(Napi::Value()).IsFunction()) {
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    Napi::Function then = value->As<Napi::Object>().Get("then").Unwrap().As<Napi::Function>();
#else
  } else if (value->IsObject() &&
             value->As<Napi::Object>().Get("then").IsFunction()) {
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    Napi::Function then = value->As<Napi::Object>().Get("then").As<Napi::Function>();
#endif
    promise = deferred.Promise();
    then.Call(
        *value,
        {Napi::Function::New(env,
                       [deferred](const Napi::CallbackInfo& info) -> Napi::Value {
                         deferred.Resolve(info[0]);
                         return info.Env().Undefined();
                       }),
         Napi::Function::New(env, [deferred](const Napi::CallbackInfo& info) -> Napi::Value {
           deferred.Reject(info[0]);
           return info.Env().Undefined();
         })});
  } else {
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    deferred.Resolve(*value);
    promise = deferred.Promise();
  }
#ifdef NODE_ADDON_API_ENABLE_MAYBE
  Napi::Function then = promise.As<Napi::Promise>().Get("then").Unwrap().As<Napi::Function>();
#else
  Napi::Function then = promise.As<Napi::Promise>().Get("then").As<Napi::Function>();
#endif

  then.Call(promise,
            {Napi::Function::New(env, OnFulfill, nullptr, this),
             Napi::Function::New(env, OnReject, nullptr, this)});
}

inline Napi::Value CoPromise::Awaiter::await_resume() const {
  const Napi::Value* ok = std::get_if<1>(&state_);
  if (ok) {
    return *ok;
  }

  const Napi::Value* err = std::get_if<2>(&state_);
  NAPI_THROW(Napi::Error(err->Env(), *err), Napi::Value());
}

inline void CoPromise::Awaiter::Fulfill(Napi::Value) {
  handle_.resume();
}

inline void CoPromise::Awaiter::Reject(Napi::Value reason) {
  if (enabled_exceptions_) {
    handle_.resume();
  } else {
    std::coroutine_handle<promise_type>::from_address(handle_.address())
        .promise()
        .Reject(reason);
    handle_.destroy();
  }
}

inline Napi::Value CoPromise::Awaiter::OnFulfill(const Napi::CallbackInfo& info) {
  CoPromise::Awaiter* awaiter = static_cast<CoPromise::Awaiter*>(info.Data());
  Napi::Env env = info.Env();
  Napi::Value value = info[0];
  awaiter->state_.emplace<1>(value);
  awaiter->Fulfill(value);
  return env.Undefined();
}

inline Napi::Value CoPromise::Awaiter::OnReject(const Napi::CallbackInfo& info) {
  CoPromise::Awaiter* awaiter = static_cast<CoPromise::Awaiter*>(info.Data());
  Napi::Env env = info.Env();
  Napi::Value reason = info[0];
  awaiter->state_.emplace<2>(reason);
  awaiter->Reject(reason);
  return env.Undefined();
}

}  // namespace naaco

#endif