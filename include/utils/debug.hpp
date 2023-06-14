#include "common/resource.hpp"
#include "debug.fbs.hpp"
#include <map>
#include <set>
#include <vector>

namespace prime::utils {
struct ResourceDebug {
  common::ResourceHash AddRef(std::string_view path_, uint32 clHash) {
    auto resHash = common::MakeHash<char>(path_);
    resHash.type = clHash;
    std::string path(path_);

    if (uniqueRefs.contains(path)) {
      refTypes.emplace(resHash, uniqueRefs.at(path));
    } else {
      uniqueRefs.emplace(path, refs.size());
      refTypes.emplace(resHash, refs.size());
      refs.emplace_back(std::move(path));
    }

    return resHash;
  }

  std::string_view AddString(std::string_view str) {
    strings.emplace(str);
    return str;
  }

  flatbuffers::Offset<common::DebugInfo>
  Build(flatbuffers::FlatBufferBuilder &builder) {
    decltype(builder.CreateVectorOfStrings({})) strsPtr;
    flatbuffers::uoffset_t refTypesPtr;
    decltype(strsPtr) refsPtr;

    if (!refTypes.empty()) {
      builder.StartVector(refTypes.size(), sizeof(common::ReferenceRemap));
      for (auto &r : refTypes) {
        common::ReferenceRemap item{r.first.type, r.second};
        builder.PushBytes(reinterpret_cast<uint8 *>(&item), sizeof(item));
      }
      refTypesPtr = builder.EndVector(refTypes.size());
      refsPtr = builder.CreateVectorOfStrings(refs);
    }

    if (!strings.empty()) {
      std::vector<std::string> stringsVec(strings.begin(), strings.end());
      strsPtr = builder.CreateVectorOfStrings(stringsVec);
    }

    common::DebugInfoBuilder debugBuilder(builder);
    if (!refTypes.empty()) {
      debugBuilder.add_references(refsPtr);
      debugBuilder.add_refTypes(refTypesPtr);
    }

    if (!strings.empty()) {
      debugBuilder.add_strings(strsPtr);
    }

    return debugBuilder.Finish();
  }

private:
  std::map<std::string, size_t> uniqueRefs;
  std::vector<std::string> refs;
  std::set<std::string> strings;
  std::map<common::ResourceHash, uint32> refTypes;
};
} // namespace prime::utils
