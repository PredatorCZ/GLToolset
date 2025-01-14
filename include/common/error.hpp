#pragma once
#include "spike/util/supercore.hpp"

#define NO_ERROR                                                               \
  prime::common::ReturnStatus { 0 }
#define ERROR_RUNTIME                                                          \
  prime::common::ReturnStatus { 1 }
#define ERROR_KEY_NOT_FOUND_IN_MAP                                             \
  prime::common::ReturnStatus { 2 }

namespace prime::common {
struct ReturnStatus {
  int16 value = 0;

  constexpr operator bool() const { return value; }
};

template <class C>
concept IsReturnType = !std::is_void_v<C> && requires(C t) {
  { t.status } -> std::same_as<ReturnStatus &>;
  t.retVal;
};

template <class C>
concept NotReturnType = !IsReturnType<C>;

template <class... C>
void ErrorImpl(const char *fmt [[maybe_unused]], C &&...args [[maybe_unused]]) {
}
inline void ErrorImpl(const char *msg [[maybe_unused]]) {}

template <class... C>
ReturnStatus Error_(ReturnStatus type, const char *fmt, C &&...args) {
  ErrorImpl(fmt, std::forward<C>(args)...);
  return type;
}

template <class... C> ReturnStatus RuntimeError(const char *fmt, C &&...args) {
  ErrorImpl(fmt, std::forward<C>(args)...);
  return {ERROR_RUNTIME};
}

inline ReturnStatus RuntimeError(const char *msg) {
  ErrorImpl(msg);
  return {ERROR_RUNTIME};
}

#define RUNTIME_ERROR(...) prime::common::RuntimeError(__VA_ARGS__)
// prime::common::RuntimeError("%s: " fmt, __PRETTY_FUNCTION__, __VA_ARGS__)

template <class C> struct [[nodiscard]] Return;

struct ReturnTraits {
  template <class F>
  static constexpr bool IsVoidRetn = std::is_void_v<std::invoke_result_t<F>>;

  template <IsReturnType F> static F AsReturn_();
  template <NotReturnType F> static Return<F> AsReturn_();

  template <class F> using AsReturn = decltype(AsReturn_<F>());

  template <IsReturnType F> static bool IsReturn_();
  template <NotReturnType F> static void IsReturn_();

  template <class F>
  static constexpr bool IsReturn =
      !std::is_void_v<std::invoke_result_t<decltype(IsReturn_<F>)>>;
};

template <class C> struct [[nodiscard]] Return {
  ReturnStatus status{};
  C retVal{};

  using T = ReturnTraits;

  void Unused() {}

  Return<void> Void();

  template <class F> C ValueOr(F &&onFail) {
    if (status) [[unlikely]] {
      if constexpr (T::template IsVoidRetn<F>) {
        onFail();
      } else {
        return onFail();
      }
    }

    return retVal;
  }

  template <class F> auto Invoke(F &&fn);

  template <class F1, class F2> auto Either(F1 &&onSuccess, F2 &&onFail) {
    if (status) [[unlikely]] {
      return Invoke(onFail);
    } else {
      return Invoke(onSuccess);
    }
  }

  template <class F> auto Success(F &&onSuccess) { return Invoke(onSuccess); }
};

static_assert(ReturnTraits::IsReturn<void> == false);
static_assert(ReturnTraits::IsReturn<Return<char>> == true);

template <> struct [[nodiscard]] Return<void> {
  ReturnStatus status;
  using T = ReturnTraits;

  void Unused() {}

  template <class F1, class F2> auto Either(F1 &&onSuccess, F2 &&onFail) {
    if (status) [[unlikely]] {
      return Invoke(onFail);
    } else {
      return Invoke(onSuccess);
    }
  }

  template <class F> auto Success(F &&onSuccess) { return Invoke(onSuccess); }

  template <class F> auto Invoke(F &&fn) {
    using IR = std::decay_t<std::invoke_result_t<F>>;
    if constexpr (std::is_void_v<IR>) {
      fn();
      return *this;
    } else if constexpr (std::is_same_v<IR, ReturnStatus>) {
      return Return<void>{fn()};
    } else if constexpr (T::template IsReturn<IR>) {
      return fn();
    } else {
      return Return<IR>{status, fn()};
    }
  }
};

template <class C>
prime::common::Return<void> prime::common::Return<C>::Void() {
  return {status};
}

template <class C>
template <class F>
auto prime::common::Return<C>::Invoke(F &&fn) {
  if constexpr (std::is_invocable_v<F, C&>) {
    using IR = std::decay_t<std::invoke_result_t<F, C&>>;
    if constexpr (std::is_void_v<IR>) {
      fn(retVal);
      return *this;
    } else if constexpr (std::is_same_v<IR, ReturnStatus>) {
      return Return<C>{fn(retVal), this->retVal};
    } else if constexpr (T::template IsReturn<IR>) {
      return fn(retVal);
    } else {
      return Return<IR>{status, fn(retVal)};
    }
  } else {
    using IR = std::decay_t<std::invoke_result_t<F>>;
    if constexpr (std::is_void_v<IR>) {
      fn();
      return *this;
    } else if constexpr (std::is_same_v<IR, ReturnStatus>) {
      return Return<C>{fn(), this->retVal};
    } else if constexpr (T::template IsReturn<IR>) {
      return fn();
    } else {
      return Return<IR>{status, fn()};
    }
  }
}
} // namespace prime::common
