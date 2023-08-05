#include "native/model_single.fbs.hpp"
#include "native/vertex.fbs.hpp"
#include "spike/crypto/jenkinshash.hpp"
#include "spike/io/binreader_stream.hpp"
#include <map>

template <class FBClass> std::string StripResource(BinReaderRef rd) {
  std::string buffer;
  rd.ReadContainer(buffer, rd.GetSize());
  FBClass *fbuffer = flatbuffers::GetMutableRoot<FBClass>(buffer.data());
  typename FBClass::NativeTableType native;
  fbuffer->UnPackTo(&native);
  es::Dispose(native.debugInfo);

  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(FBClass::Pack(builder, &native));
  return {reinterpret_cast<char *>(builder.GetBufferPointer()),
          builder.GetSize()};
}

std::map<uint32, std::string (*)(BinReaderRef)> STRIPPERS{
    {JenkinsHash_("prime::graphics::ModelSingle"),
     StripResource<prime::graphics::ModelSingle>},
    {JenkinsHash_("prime::graphics::VertexArray"),
     StripResource<prime::graphics::VertexArray>},
};
