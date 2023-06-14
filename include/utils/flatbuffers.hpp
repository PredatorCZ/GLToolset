#include "common/resource.hpp"
#include <flatbuffers/flatbuffers.h>

namespace prime::utils {
template <class C> void FinishFlatBuffer(C &builder) {
  using TableType = C::Table;

  auto &fbb = builder.fbb_;
  auto res = prime::common::ClassResource<TableType>();
  builder.add_meta(&res);

  auto ext = prime::common::GetClassExtension<TableType>();
  ext.c[4] = 0;

  fbb.Finish(builder.Finish(), ext.c);
}

template <class T>
using fbs_has_debug = decltype(std::declval<T>().debugInfo());
template <class C>
constexpr bool fbs_has_debug_v = es::is_detected_v<fbs_has_debug, C>;

template <class C> void LoadRefs(C *fbs) {
  if (auto debug = fbs->debugInfo(); debug && debug->references()) {
    auto &refNames = *debug->references();
    auto &refTypes = *debug->refTypes();
    for (auto ref : refTypes) {
      const char *resourcePath = refNames[ref->index()]->c_str();
      common::AddSimpleResource(resourcePath, ref->type());
    }
  }
}

template <class C> C *GetFlatbuffer(std::string &data) {
  auto cls = flatbuffers::GetMutableRoot<C>(data.data());
  auto ext = common::GetClassExtension<C>();
  common::ExtString expected{};
  memcpy(expected.c, flatbuffers::GetBufferIdentifier(data.data()), 4);

  if (uint32(ext.raw) != uint32(expected.raw)) {
    throw std::runtime_error("Invalid resource identifier");
  }

  if (*cls->meta() != common::ClassResource<C>()) {
    throw std::runtime_error("Invalid resource class");
  }

  if constexpr (fbs_has_debug_v<C>) {
    LoadRefs(cls);
  }

  return cls;
}

template <class C> C *GetFlatbuffer(common::ResourceData &data) {
  return GetFlatbuffer<C>(data.buffer);
}

} // namespace prime::utils
