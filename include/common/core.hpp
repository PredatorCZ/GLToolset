#pragma once
#include "spike/crypto/jenkinshash.hpp"
#include "spike/except.hpp"
#include <string_view>

#define HASH_CLASS(...)                                                        \
  template <> constexpr uint32 prime::common::GetClassHash<__VA_ARGS__>() {    \
    return JenkinsHash_(#__VA_ARGS__);                                         \
  }                                                                            \
  template <>                                                                  \
  constexpr std::string_view prime::common::GetClassName<__VA_ARGS__>() {      \
    return #__VA_ARGS__;                                                       \
  }

#define CLASS_EXT(...)                                                         \
  HASH_CLASS(__VA_ARGS__)                                                      \
  template <>                                                                  \
  constexpr prime::common::ExtString                                           \
  prime::common::GetClassExtension<__VA_ARGS__>() {                            \
    return ExtensionFromClassName(#__VA_ARGS__);                               \
  }

#define CLASS_VERSION(version)                                                 \
  common::Resource {                                                           \
    common::GetClassHash<std::decay_t<decltype(*this)>>(), version             \
  }

#define CLASS_RESOURCE(version_, ...)                                          \
  CLASS_EXT(__VA_ARGS__)                                                       \
  template <>                                                                  \
  constexpr prime::common::Resource                                            \
  prime::common::ClassResource<__VA_ARGS__>() {                                \
    return Resource{.hash = GetClassHash<__VA_ARGS__>(), .version = version_}; \
  }

namespace prime::common {
template <class C> constexpr uint32 GetClassHash() { return 0; }
template <> constexpr uint32 GetClassHash<char>() {
  return JenkinsHash_("prime::common::String");
}

struct Resource {
  uint32 hash;
  uint32 version;

  constexpr bool operator!=(Resource o) const {
    return hash != o.hash || version != o.version;
  }
};

template <class C> constexpr Resource ClassResource();

template <class C> constexpr std::string_view GetClassName();

union ExtString {
  uint64 raw = 0;
  char c[8];
  constexpr ExtString() = default;
  constexpr ExtString(uint64 r) : raw(r) {}
  constexpr operator std::string_view() const { return c; }
  constexpr bool operator<(ExtString o) const { return raw < o.raw; }
};

template <class C> constexpr ExtString GetClassExtension() { return {}; }

consteval uint64 ExtensionFromClassName(std::string_view fullName) {
  uint64 buffa = fullName[7];
  const size_t startGen = fullName.find_last_of(':');

  size_t numUpperCases = 1;
  size_t lastUpperCase = 0;

  for (size_t g = startGen; g < fullName.size(); g++) {
    if (fullName[g] >= 'A' && fullName[g] <= 'Z') {
      buffa |= uint64(fullName[g] + 32) << numUpperCases++ * 8;
      lastUpperCase = g;
    } else if (fullName[g] >= '0' && fullName[g] <= '9') {
      buffa |= uint64(fullName[g]) << numUpperCases++ * 8;
      lastUpperCase = g;
    }

    if (numUpperCases >= 6) {
      break;
    }
  }

  if (numUpperCases < 3) {
    lastUpperCase = lastUpperCase + (fullName.size() - lastUpperCase) / 2;
    while (true) {
      if (fullName[lastUpperCase] >= 'A' && fullName[lastUpperCase] <= 'Z') {
        break;
      }

      if (fullName[lastUpperCase] >= 'a' && fullName[lastUpperCase] <= 'z') {
        break;
      }

      if (fullName[lastUpperCase] >= '0' && fullName[lastUpperCase] <= '9') {
        break;
      }

      lastUpperCase++;
    }

    buffa |= uint64(fullName[lastUpperCase]) << 16;
  }

  if (numUpperCases < 4) {
    lastUpperCase = fullName.size() - 1;
    while (true) {
      if (fullName[lastUpperCase] >= 'A' && fullName[lastUpperCase] <= 'Z') {
        break;
      }

      if (fullName[lastUpperCase] >= 'a' && fullName[lastUpperCase] <= 'z') {
        break;
      }

      if (fullName[lastUpperCase] >= '0' && fullName[lastUpperCase] <= '9') {
        break;
      }

      lastUpperCase--;
    }

    buffa |= uint64(fullName[lastUpperCase]) << 24;
  }

  return buffa;
}

template <class C> void ValidateClass(const C &item) {
  const C dummy;
  if (item.hash != dummy.hash) {
    throw es::InvalidHeaderError(item.hash);
  }

  if (item.version != dummy.version) {
    throw es::InvalidVersionError(item.version);
  }
}

uint32 GetClassFromExtension(std::string_view ext);
std::string_view GetExtentionFromHash(uint32 hash);

namespace detail {
uint32 RegisterClass(ExtString ext, uint32 obj);
template <class C> class RegistryInvokeGuard;
} // namespace detail
} // namespace prime::common

#define REGISTER_CLASS(...)                                                    \
  template <> class prime::common::detail::RegistryInvokeGuard<__VA_ARGS__> {  \
    static inline const uint32 data =                                          \
        RegisterClass(prime::common::GetClassExtension<__VA_ARGS__>(),         \
                      prime::common::GetClassHash<__VA_ARGS__>());             \
  }
