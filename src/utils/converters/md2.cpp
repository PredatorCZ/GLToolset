#include "mikktspace.h"
#include "spike/app_context.hpp"
#include "spike/crypto/crc32.hpp"
#include "spike/io/binreader_stream.hpp"
#include "spike/io/binwritter_stream.hpp"
#include "spike/master_printer.hpp"
#include "spike/type/matrix44.hpp"
#include "spike/type/pointer.hpp"
#include "spike/type/vectors_simd.hpp"
#include <map>
#include <vector>

#include <GL/gl.h>
#include <GL/glext.h>

#include "common/aabb.hpp"
#include "graphics/detail/model_single.hpp"
#include "graphics/detail/vertex_array.hpp"
#include "graphics/sampler.hpp"
#include "graphics/texture.hpp"
#include "utils/debug.hpp"
#include "utils/instancetm_builder.hpp"
#include "utils/playground.hpp"
#include "utils/shader_preprocessor.hpp"

namespace MD2 {
struct Skin {
  char path[64];
};

struct TexCoord {
  int16 uv[2];
};

struct Triangle {
  uint16 vertex[3];
  uint16 texCoord[3];
};

struct Vertex {
  UCVector pos;
  uint8 normalIndex;
};

struct Frame {
  Vector scale;
  Vector offset;
  char name[16];
  Vertex vertices[1];
};

struct CommandPacket {
  Vector2 uv;
  uint32 vertexIndex;
};

struct Command {
  uint16 numVerts;
  bool isFan;
  CommandPacket packets[1];

  Command *Next() {
    Command *nextCommand = reinterpret_cast<Command *>(end());
    if (nextCommand->numVerts == 0) {
      return nullptr;
    }

    return nextCommand;
  }

  CommandPacket *begin() { return packets; }
  CommandPacket *end() { return packets + numVerts; }
};

struct Header {
  static constexpr uint32 ID = CompileFourCC("IDP2");
  static constexpr uint32 VERSION = 8;
  uint32 id;
  uint32 version;
  uint32 skinWidth;
  uint32 skinHeight;
  uint32 frameSize;
  uint32 numSkins;
  uint32 numVertices;
  uint32 numTexCoords;
  uint32 numTris;
  uint32 numCommands; // num uint32 in command block
  uint32 numFrames;
  es::PointerX86<Skin> skins;
  es::PointerX86<TexCoord> texCoords;
  es::PointerX86<Triangle> tris;
  es::PointerX86<Frame> frames;
  es::PointerX86<Command> commands;
  uint32 fileSize;
};

void ProcessFile(Header &item) {
  if (item.id != Header::ID) {
    throw es::InvalidHeaderError(item.id);
  }

  if (item.version != Header::VERSION) {
    throw es::InvalidVersionError(item.version);
  }

  if (item.numFrames > 1) {
    printwarning("Only one frame is supported");
  }

  if (item.numCommands == 0) {
    throw std::runtime_error("MD2 soesn't contain any commands");
  }

  if (item.numSkins > 1) {
    throw std::runtime_error("Only one skin is supported");
  }

  es::FixupPointers(reinterpret_cast<char *>(&item), item.skins, item.texCoords,
                    item.tris, item.frames, item.commands);

  Command *curCommand = item.commands;
  while (curCommand) {
    if (curCommand->isFan) {
      curCommand->numVerts *= -1;
    }

    curCommand = curCommand->Next();
  }
}

static const Vector NORMALS[]{
    {-0.525731f, 0.000000f, 0.850651f},   {-0.442863f, 0.238856f, 0.864188f},
    {-0.295242f, 0.000000f, 0.955423f},   {-0.309017f, 0.500000f, 0.809017f},
    {-0.162460f, 0.262866f, 0.951056f},   {0.000000f, 0.000000f, 1.000000f},
    {0.000000f, 0.850651f, 0.525731f},    {-0.147621f, 0.716567f, 0.681718f},
    {0.147621f, 0.716567f, 0.681718f},    {0.000000f, 0.525731f, 0.850651f},
    {0.309017f, 0.500000f, 0.809017f},    {0.525731f, 0.000000f, 0.850651f},
    {0.295242f, 0.000000f, 0.955423f},    {0.442863f, 0.238856f, 0.864188f},
    {0.162460f, 0.262866f, 0.951056f},    {-0.681718f, 0.147621f, 0.716567f},
    {-0.809017f, 0.309017f, 0.500000f},   {-0.587785f, 0.425325f, 0.688191f},
    {-0.850651f, 0.525731f, 0.000000f},   {-0.864188f, 0.442863f, 0.238856f},
    {-0.716567f, 0.681718f, 0.147621f},   {-0.688191f, 0.587785f, 0.425325f},
    {-0.500000f, 0.809017f, 0.309017f},   {-0.238856f, 0.864188f, 0.442863f},
    {-0.425325f, 0.688191f, 0.587785f},   {-0.716567f, 0.681718f, -0.147621f},
    {-0.500000f, 0.809017f, -0.309017f},  {-0.525731f, 0.850651f, 0.000000f},
    {0.000000f, 0.850651f, -0.525731f},   {-0.238856f, 0.864188f, -0.442863f},
    {0.000000f, 0.955423f, -0.295242f},   {-0.262866f, 0.951056f, -0.162460f},
    {0.000000f, 1.000000f, 0.000000f},    {0.000000f, 0.955423f, 0.295242f},
    {-0.262866f, 0.951056f, 0.162460f},   {0.238856f, 0.864188f, 0.442863f},
    {0.262866f, 0.951056f, 0.162460f},    {0.500000f, 0.809017f, 0.309017f},
    {0.238856f, 0.864188f, -0.442863f},   {0.262866f, 0.951056f, -0.162460f},
    {0.500000f, 0.809017f, -0.309017f},   {0.850651f, 0.525731f, 0.000000f},
    {0.716567f, 0.681718f, 0.147621f},    {0.716567f, 0.681718f, -0.147621f},
    {0.525731f, 0.850651f, 0.000000f},    {0.425325f, 0.688191f, 0.587785f},
    {0.864188f, 0.442863f, 0.238856f},    {0.688191f, 0.587785f, 0.425325f},
    {0.809017f, 0.309017f, 0.500000f},    {0.681718f, 0.147621f, 0.716567f},
    {0.587785f, 0.425325f, 0.688191f},    {0.955423f, 0.295242f, 0.000000f},
    {1.000000f, 0.000000f, 0.000000f},    {0.951056f, 0.162460f, 0.262866f},
    {0.850651f, -0.525731f, 0.000000f},   {0.955423f, -0.295242f, 0.000000f},
    {0.864188f, -0.442863f, 0.238856f},   {0.951056f, -0.162460f, 0.262866f},
    {0.809017f, -0.309017f, 0.500000f},   {0.681718f, -0.147621f, 0.716567f},
    {0.850651f, 0.000000f, 0.525731f},    {0.864188f, 0.442863f, -0.238856f},
    {0.809017f, 0.309017f, -0.500000f},   {0.951056f, 0.162460f, -0.262866f},
    {0.525731f, 0.000000f, -0.850651f},   {0.681718f, 0.147621f, -0.716567f},
    {0.681718f, -0.147621f, -0.716567f},  {0.850651f, 0.000000f, -0.525731f},
    {0.809017f, -0.309017f, -0.500000f},  {0.864188f, -0.442863f, -0.238856f},
    {0.951056f, -0.162460f, -0.262866f},  {0.147621f, 0.716567f, -0.681718f},
    {0.309017f, 0.500000f, -0.809017f},   {0.425325f, 0.688191f, -0.587785f},
    {0.442863f, 0.238856f, -0.864188f},   {0.587785f, 0.425325f, -0.688191f},
    {0.688191f, 0.587785f, -0.425325f},   {-0.147621f, 0.716567f, -0.681718f},
    {-0.309017f, 0.500000f, -0.809017f},  {0.000000f, 0.525731f, -0.850651f},
    {-0.525731f, 0.000000f, -0.850651f},  {-0.442863f, 0.238856f, -0.864188f},
    {-0.295242f, 0.000000f, -0.955423f},  {-0.162460f, 0.262866f, -0.951056f},
    {0.000000f, 0.000000f, -1.000000f},   {0.295242f, 0.000000f, -0.955423f},
    {0.162460f, 0.262866f, -0.951056f},   {-0.442863f, -0.238856f, -0.864188f},
    {-0.309017f, -0.500000f, -0.809017f}, {-0.162460f, -0.262866f, -0.951056f},
    {0.000000f, -0.850651f, -0.525731f},  {-0.147621f, -0.716567f, -0.681718f},
    {0.147621f, -0.716567f, -0.681718f},  {0.000000f, -0.525731f, -0.850651f},
    {0.309017f, -0.500000f, -0.809017f},  {0.442863f, -0.238856f, -0.864188f},
    {0.162460f, -0.262866f, -0.951056f},  {0.238856f, -0.864188f, -0.442863f},
    {0.500000f, -0.809017f, -0.309017f},  {0.425325f, -0.688191f, -0.587785f},
    {0.716567f, -0.681718f, -0.147621f},  {0.688191f, -0.587785f, -0.425325f},
    {0.587785f, -0.425325f, -0.688191f},  {0.000000f, -0.955423f, -0.295242f},
    {0.000000f, -1.000000f, 0.000000f},   {0.262866f, -0.951056f, -0.162460f},
    {0.000000f, -0.850651f, 0.525731f},   {0.000000f, -0.955423f, 0.295242f},
    {0.238856f, -0.864188f, 0.442863f},   {0.262866f, -0.951056f, 0.162460f},
    {0.500000f, -0.809017f, 0.309017f},   {0.716567f, -0.681718f, 0.147621f},
    {0.525731f, -0.850651f, 0.000000f},   {-0.238856f, -0.864188f, -0.442863f},
    {-0.500000f, -0.809017f, -0.309017f}, {-0.262866f, -0.951056f, -0.162460f},
    {-0.850651f, -0.525731f, 0.000000f},  {-0.716567f, -0.681718f, -0.147621f},
    {-0.716567f, -0.681718f, 0.147621f},  {-0.525731f, -0.850651f, 0.000000f},
    {-0.500000f, -0.809017f, 0.309017f},  {-0.238856f, -0.864188f, 0.442863f},
    {-0.262866f, -0.951056f, 0.162460f},  {-0.864188f, -0.442863f, 0.238856f},
    {-0.809017f, -0.309017f, 0.500000f},  {-0.688191f, -0.587785f, 0.425325f},
    {-0.681718f, -0.147621f, 0.716567f},  {-0.442863f, -0.238856f, 0.864188f},
    {-0.587785f, -0.425325f, 0.688191f},  {-0.309017f, -0.500000f, 0.809017f},
    {-0.147621f, -0.716567f, 0.681718f},  {-0.425325f, -0.688191f, 0.587785f},
    {-0.162460f, -0.262866f, 0.951056f},  {0.442863f, -0.238856f, 0.864188f},
    {0.162460f, -0.262866f, 0.951056f},   {0.309017f, -0.500000f, 0.809017f},
    {0.147621f, -0.716567f, 0.681718f},   {0.000000f, -0.525731f, 0.850651f},
    {0.425325f, -0.688191f, 0.587785f},   {0.587785f, -0.425325f, 0.688191f},
    {0.688191f, -0.587785f, 0.425325f},   {-0.955423f, 0.295242f, 0.000000f},
    {-0.951056f, 0.162460f, 0.262866f},   {-1.000000f, 0.000000f, 0.000000f},
    {-0.850651f, 0.000000f, 0.525731f},   {-0.955423f, -0.295242f, 0.000000f},
    {-0.951056f, -0.162460f, 0.262866f},  {-0.864188f, 0.442863f, -0.238856f},
    {-0.951056f, 0.162460f, -0.262866f},  {-0.809017f, 0.309017f, -0.500000f},
    {-0.864188f, -0.442863f, -0.238856f}, {-0.951056f, -0.162460f, -0.262866f},
    {-0.809017f, -0.309017f, -0.500000f}, {-0.681718f, 0.147621f, -0.716567f},
    {-0.681718f, -0.147621f, -0.716567f}, {-0.850651f, 0.000000f, -0.525731f},
    {-0.688191f, 0.587785f, -0.425325f},  {-0.587785f, 0.425325f, -0.688191f},
    {-0.425325f, 0.688191f, -0.587785f},  {-0.425325f, -0.688191f, -0.587785f},
    {-0.587785f, -0.425325f, -0.688191f}, {-0.688191f, -0.587785f, -0.425325f},
};

} // namespace MD2

inline bool fltcmp(float f0, float f1, float epsilon = 0.0001) {
  return (f1 <= f0 + epsilon) && (f1 >= f0 - epsilon);
}

namespace prime::utils {
void ProcessMD2(AppContext *ctx) {
  std::string buffer = ctx->GetBuffer();
  const uint32 inputCrc = crc32b(0, buffer.data(), buffer.size());
  auto &hdr = *reinterpret_cast<MD2::Header *>(buffer.data());

  using namespace MD2;
  ProcessFile(hdr);

  struct FVertex {
    Vector2 uv;
    Vector normal;
    UCVector position;

    bool operator<(const FVertex &o) const {
      if (fltcmp(uv.x, o.uv.x)) {
        if (fltcmp(uv.y, o.uv.y)) {
          if (fltcmp(normal.x, o.normal.x)) {
            if (fltcmp(normal.y, o.normal.y)) {
              if (fltcmp(normal.z, o.normal.z)) {
                if (position.x == o.position.x) {
                  if (position.y == o.position.y) {
                    return position.z < o.position.z;
                  }
                  return position.y < o.position.y;
                }
                return position.x < o.position.x;
              }
              return normal.z < o.normal.z;
            }
            return normal.y < o.normal.y;
          }
          return normal.x < o.normal.x;
        }
        return uv.y < o.uv.y;
      }
      return uv.x < o.uv.x;
    }
  };

  std::map<FVertex, uint16> indexedVertices;
  std::vector<uint16> indices;

  Command *curCommand = hdr.commands;
  Frame *frame = hdr.frames;
  Vertex *vertices = frame->vertices;

  auto MakeVertex = [vertices](CommandPacket &p) {
    FVertex vt;
    vt.uv = p.uv;
    auto &vtx = vertices[p.vertexIndex];
    vt.position = vtx.pos;
    vt.normal = NORMALS[vtx.normalIndex];
    return vt;
  };

  auto GetVertexIndex = [&](FVertex &v) {
    if (indexedVertices.count(v)) {
      return indexedVertices.at(v);
    }

    const uint16 newId = indexedVertices.size();
    indexedVertices.emplace(v, newId);
    return newId;
  };

  while (curCommand) {
    if (curCommand->isFan) {
      FVertex baseVertex = MakeVertex(curCommand->packets[0]);
      FVertex lastVertex = MakeVertex(curCommand->packets[1]);

      for (auto b = curCommand->begin() + 2; b < curCommand->end(); b++) {
        auto index0 = GetVertexIndex(lastVertex);
        auto index1 = GetVertexIndex(baseVertex);
        lastVertex = MakeVertex(*b);
        auto index2 = GetVertexIndex(lastVertex);
        indices.emplace_back(index0);
        indices.emplace_back(index1);
        indices.emplace_back(index2);
      }

    } else {
      FVertex vertex0 = MakeVertex(curCommand->packets[0]);
      FVertex vertex1 = MakeVertex(curCommand->packets[1]);
      bool odd = false;

      for (auto b = curCommand->begin() + 2; b < curCommand->end(); b++) {
        auto index0 = GetVertexIndex(vertex1);
        auto index1 = GetVertexIndex(vertex0);
        FVertex curVertex = MakeVertex(*b);
        auto index2 = GetVertexIndex(curVertex);

        indices.emplace_back(index0);
        indices.emplace_back(index1);
        indices.emplace_back(index2);

        if (odd) {
          vertex1 = curVertex;
        } else {
          vertex0 = curVertex;
        }

        odd = !odd;
      }
    }

    curCommand = curCommand->Next();
  }

  std::vector<FVertex> oVertices;
  oVertices.resize(indexedVertices.size());

  for (auto [v, i] : indexedVertices) {
    oVertices[i] = v;
  }

  es::Dispose(indexedVertices);

  struct MikkUserData {
    Vector4A16 scale;
    Vector4A16 offset;
    std::vector<FVertex> &vertices;
    std::vector<uint16> &indices;
    std::vector<SVector4> tangents;
    Vector4A16 min;
    Vector4A16 max;
  } uData{Vector4A16(frame->scale, 0),
          Vector4A16(frame->offset, 0),
          oVertices,
          indices,
          {},
          {FLT_MAX},
          {-FLT_MAX}};

  uData.tangents.resize(oVertices.size());

  SMikkTSpaceContext tctx{};
  tctx.m_pUserData = &uData;
  SMikkTSpaceInterface it{};
  it.m_getNumFaces = [](const SMikkTSpaceContext *c) -> int {
    return static_cast<MikkUserData *>(c->m_pUserData)->indices.size() / 3;
  };
  it.m_getNumVerticesOfFace = [](const SMikkTSpaceContext *, int) { return 3; };
  it.m_getPosition = [](const SMikkTSpaceContext *c, float *out, int face,
                        int vert) {
    auto uData = static_cast<MikkUserData *>(c->m_pUserData);
    const uint16 index = uData->indices.at(face * 3 + vert);
    Vector4A16 pos = uData->vertices.at(index).position.Convert<float>();
    pos = (pos * uData->scale) + uData->offset;
    memcpy(out, &pos, sizeof(Vector));

    uData->max._data = _mm_max_ps(uData->max._data, pos._data);
    uData->min._data = _mm_min_ps(uData->min._data, pos._data);
  };

  it.m_getNormal = [](const SMikkTSpaceContext *c, float *out, int face,
                      int vert) {
    auto uData = static_cast<MikkUserData *>(c->m_pUserData);
    const uint16 index = uData->indices.at(face * 3 + vert);
    Vector norm = uData->vertices.at(index).normal;
    memcpy(out, &norm, sizeof(norm));
  };

  it.m_getTexCoord = [](const SMikkTSpaceContext *c, float *out, int face,
                        int vert) {
    auto uData = static_cast<MikkUserData *>(c->m_pUserData);
    const uint16 index = uData->indices.at(face * 3 + vert);
    Vector2 uv = uData->vertices.at(index).uv;
    memcpy(out, &uv, sizeof(uv));
  };

  it.m_setTSpaceBasic = [](const SMikkTSpaceContext *c, const float *fvTangent,
                           float fSign, int face, int vert) {
    auto uData = static_cast<MikkUserData *>(c->m_pUserData);
    uint16 &index = uData->indices.at(face * 3 + vert);
    es::Matrix44 mtx;
    mtx.r3() = uData->vertices.at(index).normal;
    memcpy(reinterpret_cast<float *>(&mtx.r1()), fvTangent, 12);

    // orthogonalize tangent
    mtx.r1() -= mtx.r3() * mtx.r1().DotV(mtx.r3());
    mtx.r1().Normalize();
    mtx.r2() = mtx.r1().Cross(mtx.r3()) * fSign;
    Vector4A16 t, tang, s;
    mtx.Decompose(t, tang, s);

    // quat rule: q == -q
    if (tang.w < 0) {
      tang *= -1;
    }

    // Set reflection by flipping quat
    if (s.x * s.y * s.z < 0) {
      tang *= -1;
    }

    // Opengl < 4.2
    // https://www.khronos.org/opengl/wiki/Normalized_Integer#Alternate_mapping
    tang = (tang * 0xffff - 1) / 2;
    tang = Vector4A16(_mm_round_ps(tang._data, _MM_ROUND_NEAREST));

    // Apply bias needed for reflection
    if (tang.w == 0) {
      tang.w = s.x * s.y * s.z;
    }

    // Backward check for normal correctness
    auto pTang = (tang * 2 + 1) / 0xffff;
    glm::quat qq(pTang.w, pTang.x, pTang.y, pTang.z);

    {
      auto oNorm = mtx.r3();
      glm::vec3 origNormal(oNorm.x, oNorm.y, oNorm.z);

      glm::vec3 normalLoc(0, 0, qq.w < 0 ? -1 : 1);
      glm::vec3 normalFromQuat = qq * normalLoc;
      glm::vec3 normalDiff = normalFromQuat - origNormal;

      float normalDiffTotal =
          abs(normalDiff.x) + abs(normalDiff.y) + abs(normalDiff.z);

      if (normalDiffTotal > 0.1) {
        printline(normalDiffTotal);
      }
    }

    {
      auto oTang = mtx.r1();
      glm::vec3 origTangent(oTang.x, oTang.y, oTang.z);

      auto pTang = (tang * 2 + 1) / 0xffff;
      glm::quat qq(pTang.w, pTang.x, pTang.y, pTang.z);

      glm::vec3 tangentLoc(qq.w < 0 ? -1 : 1, 0, 0);
      glm::vec3 tangentFromQuat = qq * tangentLoc;
      glm::vec3 tangentDiff = tangentFromQuat - origTangent;

      float tangentDiffTotal =
          abs(tangentDiff.x) + abs(tangentDiff.y) + abs(tangentDiff.z);

      if (tangentDiffTotal > 0.1) {
        printline(tangentDiffTotal);
      }
    }

    {
      auto oTang = mtx.r2();
      glm::vec3 origTangent(oTang.x, oTang.y, oTang.z);

      auto pTang = (tang * 2 + 1) / 0xffff;
      glm::quat qq(pTang.w, pTang.x, pTang.y, pTang.z);

      glm::vec3 tangentLoc(0, qq.w < 0 ? -1 : 1, 0);
      glm::vec3 tangentFromQuat = qq * tangentLoc;
      glm::vec3 tangentDiff = tangentFromQuat - origTangent;

      float tangentDiffTotal =
          abs(tangentDiff.x) + abs(tangentDiff.y) + abs(tangentDiff.z);

      if (tangentDiffTotal > 0.1) {
        printline(tangentDiffTotal);
      }
    }

    auto &oTang = uData->tangents.at(index);
    auto nTang = tang.Convert<int16>();

    if (oTang == SVector4{}) {
      oTang = nTang;
    } else if (oTang != nTang) {
      auto copiedVert = uData->vertices.at(index);
      index = uData->vertices.size();
      uData->vertices.emplace_back(copiedVert);
      uData->tangents.emplace_back(nTang);

      // Print out value in case of close values
      auto v = oTang - nTang;
      if ((abs(v.x) + abs(v.y) + abs(v.z) + abs(v.w)) < 1000) {
        printline(v.X << " " << v.Y << " " << v.Z << " " << v.W);
      }
    }
  };

  tctx.m_pInterface = &it;

  genTangSpaceDefault(&tctx);

  namespace pg = prime::graphics;
  namespace pc = prime::common;
  namespace pu = prime::utils;

  prime::utils::ResourceDebugPlayground debugVayPg;
  debugVayPg.main->inputCrc = inputCrc;

  // Dump position buffer
  auto outFile = std::string(ctx->workingFile.GetFullPathNoExt());
  pc::ResourceHash posHash;
  outFile += "." + std::string(pc::GetClassExtension<pg::VertexVshData>());
  AFileInfo outPath;
  std::string outDir;

  {
    auto nctx = ctx->NewFile(outFile);
    outPath.Load(nctx.fullPath.data() + nctx.delimiter);
    outDir = nctx.fullPath.substr(0, nctx.delimiter);
    posHash = debugVayPg.AddRef(outPath.GetFullPathNoExt(),
                                pc::GetClassHash<pg::VertexVshData>());
    BinWritterRef wrh(nctx.str);

    for (auto &v : oVertices) {
      wrh.Write(UCVector4(v.position.x, v.position.y, v.position.z, 0));
    }
  }

  // Dump index buffer
  outFile = std::string(ctx->workingFile.GetFullPathNoExt());
  pc::ResourceHash indicesHash;
  outFile += "." + std::string(pc::GetClassExtension<pg::VertexIndexData>());

  {
    auto nctx = ctx->NewFile(outFile);
    indicesHash = debugVayPg.AddRef(outPath.GetFullPathNoExt(),
                                    pc::GetClassHash<pg::VertexIndexData>());
    BinWritterRef wrh(nctx.str);
    wrh.WriteContainer(indices);
  }

  Vector2 uvMax, uvMin;

  for (auto &v : oVertices) {
    uvMax.x = std::max(uvMax.x, v.uv.x);
    uvMax.y = std::max(uvMax.y, v.uv.y);
    uvMin.x = std::min(uvMin.x, v.uv.x);
    uvMin.y = std::min(uvMin.y, v.uv.y);
  }

  // Dump pixel buffer
  outFile = std::string(ctx->workingFile.GetFullPathNoExt());
  pc::ResourceHash pixelHash;
  outFile += "." + std::string(pc::GetClassExtension<pg::VertexPshData>());

  {
    auto nctx = ctx->NewFile(outFile);
    pixelHash = debugVayPg.AddRef(outPath.GetFullPathNoExt(),
                                  pc::GetClassHash<pg::VertexPshData>());
    BinWritterRef wrh(nctx.str);

    for (size_t v = 0; v < oVertices.size(); v++) {
      SVector4 tang = uData.tangents.at(v);
      tang = SVector4(tang.w, -tang.x, -tang.y, -tang.z);
      wrh.Write(tang);
      auto ratio = (uvMin - oVertices.at(v).uv) / (uvMin - uvMax);
      ratio = ratio * 0xffff;
      ratio.x = std::min(std::round(ratio.x), float(0xffff));
      ratio.y = std::min(std::round(ratio.y), float(0xffff));
      wrh.Write(ratio.Convert<uint16>());
    }
  }

  // Create vertex arrays
  pc::ResourceHash vayHash;
  prime::utils::ResourceDebugPlayground debugPg;
  debugPg.main->inputCrc = inputCrc;
  {
    outFile =
        outPath.ChangeExtension2(pc::GetClassExtension<pg::VertexArray>());

    vayHash = debugPg.AddRef(outPath.GetFullPathNoExt(),
                             pc::GetClassHash<pg::VertexArray>());

    pu::PlayGround vtArrayPg;
    pu::PlayGround::Pointer<pg::VertexArray> vtArray(
        vtArrayPg.AddClass<pg::VertexArray>());

    {
      pu::PlayGround::Pointer<pg::VertexBuffer> curBuffer(
          vtArrayPg.ArrayEmplace(vtArray->buffers));

      vtArrayPg.ArrayEmplace(curBuffer->attributes, pg::VertexType::Position, 3,
                             GL_UNSIGNED_BYTE, false, 0);

      curBuffer->buffer = posHash;
      curBuffer->size = 4 * oVertices.size();
      curBuffer->stride = 4;
      curBuffer->target = GL_ARRAY_BUFFER;
      curBuffer->usage = GL_STATIC_DRAW;
    }

    {
      pu::PlayGround::Pointer<pg::VertexBuffer> curBuffer(
          vtArrayPg.ArrayEmplace(vtArray->buffers));

      vtArrayPg.ArrayEmplace(curBuffer->attributes, pg::VertexType::Tangent, 4,
                             GL_SHORT, true, 0);
      vtArrayPg.ArrayEmplace(curBuffer->attributes, pg::VertexType::TexCoord2,
                             2, GL_UNSIGNED_SHORT, false, 8);

      curBuffer->buffer = pixelHash;
      curBuffer->size = 12 * oVertices.size();
      curBuffer->stride = 12;
      curBuffer->target = GL_ARRAY_BUFFER;
      curBuffer->usage = GL_STATIC_DRAW;
    }

    {
      pu::PlayGround::Pointer<pg::VertexBuffer> curBuffer(
          vtArrayPg.ArrayEmplace(vtArray->buffers));

      curBuffer->buffer = indicesHash;
      curBuffer->size = 2 * indices.size();
      curBuffer->stride = 0;
      curBuffer->target = GL_ELEMENT_ARRAY_BUFFER;
      curBuffer->usage = GL_STATIC_DRAW;
    }

    vtArrayPg.ArrayEmplace(
        vtArray->transforms,
        pc::Transform{
            .tm =
                {
                    {1, 0, 0, 0},
                    {frame->offset.x, frame->offset.y, frame->offset.z},
                },
            .inflate = {frame->scale.x, frame->scale.y, frame->scale.z},
        });

    uvMax -= uvMin;
    uvMax *= 1.f / 0xffff;

    vtArrayPg.ArrayEmplace(vtArray->uvTransform, uvMin.x, uvMin.y, uvMax.x,
                           uvMax.y);
    vtArrayPg.ArrayEmplace(vtArray->uvTransforRemaps, 0);

    vtArray->count = indices.size();
    vtArray->mode = GL_TRIANGLES;
    vtArray->type = GL_UNSIGNED_SHORT;
    vtArray->index = -1;
    vtArray->tsType = pg::TSType::QTangent;

    pc::AABB aabb_{
        .center = (uData.max + uData.min) / 2,
        .bounds = uData.max,
    };
    aabb_.bounds -= aabb_.center;
    aabb_.bounds.w =
        std::max(std::max(aabb_.bounds.x, aabb_.bounds.y), aabb_.bounds.z);

    vtArray->aabb = aabb_;

    std::string built = debugVayPg.Build<pg::VertexArray>(vtArrayPg);

    BinWritterRef wrh(ctx->NewFile(outFile).str);
    wrh.WriteContainer(built);
  }

  // Create single model
  {
    pu::PlayGround modelPg;
    pu::PlayGround::Pointer<pg::ModelSingle> model(
        modelPg.AddClass<pg::ModelSingle>());
    model->program.proto = pg::LegacyProgram{};
    model->vertexArray = vayHash;

    AFileInfo textureFile(hdr.skins->path);
    AFileInfo newBranch(textureFile.CatchBranch(outFile));

    struct TextureSlot {
      JenHash3 hash;
      std::string_view slot;
    };

    std::vector<TextureSlot> textures;

    {
      auto texHash = debugPg.AddRef(newBranch.GetFullPathNoExt(),
                                    pc::GetClassHash<pg::Texture>());
      textures.emplace_back(
          TextureSlot{texHash.name, debugPg.AddString("smAlbedo")});
    }

    try {
      ctx->FindFile(outDir + std::string(newBranch.GetFolder()),
                    "^" + std::string(newBranch.GetFilename()) + "_normal.");
      auto texHash = debugPg.AddRef(newBranch.ChangeExtension("_normal"),
                                    pc::GetClassHash<pg::Texture>());
      textures.emplace_back(
          TextureSlot{texHash.name, debugPg.AddString("smNormal")});

      pu::PlayGround::Pointer<pc::String> newDef = modelPg.ArrayEmplace(
          model->program.proto.Get<pg::LegacyProgram>().definitions);
      modelPg.NewString(*newDef, "PS_IN_NORMAL");
    } catch (const es::FileNotFoundError &) {
    }

    try {
      ctx->FindFile(outDir + std::string(newBranch.GetFolder()),
                    "^" + std::string(newBranch.GetFilename()) + "_fx.");
      auto texHash = debugPg.AddRef(newBranch.ChangeExtension("_fx"),
                                    pc::GetClassHash<pg::Texture>());
      textures.emplace_back(
          TextureSlot{texHash.name, debugPg.AddString("smGlow")});

      pu::PlayGround::Pointer<pc::String> newDef = modelPg.ArrayEmplace(
          model->program.proto.Get<pg::LegacyProgram>().definitions);
      modelPg.NewString(*newDef, "PS_IN_GLOW");
    } catch (const es::FileNotFoundError &) {
    }

    modelPg.ArrayEmplace(
        model->program.proto.Get<pg::LegacyProgram>().stages,
        debugPg
            .AddRef("simple_model/main", pc::GetClassHash<pg::VertexSource>())
            .name,
        0, GL_VERTEX_SHADER);

    modelPg.ArrayEmplace(
        model->program.proto.Get<pg::LegacyProgram>().stages,
        debugPg
            .AddRef("simple_model/main", pc::GetClassHash<pg::FragmentSource>())
            .name,
        0, GL_FRAGMENT_SHADER);

    for (auto &t : textures) {
      modelPg.ArrayEmplace(
          model->textures, t.hash, debugPg.AddString(t.slot),
          debugPg.AddRef("core/default", pc::GetClassHash<pg::Sampler>()).name,
          0, 0);
    }

    modelPg.ArrayEmplace(
        model->uniformBlocks,
        debugPg.AddRef("main_uniform",
                       pc::GetClassHash<pg::UniformBlockData>()),
        JenkinsHash_(debugPg.AddString("ubFragmentProperties")), 0, 0, 0);

    outFile = ctx->workingFile.ChangeExtension2(
        pc::GetClassExtension<pg::ModelSingle>());

    std::string built = debugPg.Build<pg::ModelSingle>(modelPg);
    BinWritterRef wrh(ctx->NewFile(outFile).str);
    wrh.WriteContainer(built);
  }
}

std::span<std::string_view> ProcessMD2Filters() {
  static std::string_view filters[]{
      ".md2$",
  };

  return filters;
}
} // namespace prime::utils
