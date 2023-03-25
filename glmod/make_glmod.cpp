/*  GLMOD
    Copyright(C) 2022 Lukas Cone

    This program is free software : you can redistribute it and / or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.If not, see <https://www.gnu.org/licenses/>.
*/

#include "datas/app_context.hpp"
#include "datas/binreader_stream.hpp"
#include "datas/binwritter_stream.hpp"
#include "datas/directory_scanner.hpp"
#include "datas/fileinfo.hpp"
#include "datas/master_printer.hpp"
#include "datas/matrix44.hpp"
#include "datas/pointer.hpp"
#include "datas/reflector.hpp"
#include "datas/vectors_simd.hpp"
#include "mikktspace.h"
#include "project.h"
#include "uni/format.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

#include "utils/flatbuffers.hpp"
#include "utils/instancetm_builder.hpp"
#include "utils/shader_preprocessor.hpp"

#include "glm/ext.hpp"
#include "program.fbs.hpp"
#include "texture.fbs.hpp"
#include "vertex.fbs.hpp"

#include "model_single.fbs.hpp"

std::string_view filters[]{
    ".md2$",
};

static AppInfo_s appInfo{
    .filteredLoad = true,
    .header = GLMOD_DESC " v" GLMOD_VERSION ", " GLMOD_COPYRIGHT "Lukas Cone",
    .filters = filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

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

void Process(MD2::Header &hdr, AppContext *ctx) {
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

  std::vector<std::string> refs;

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
    posHash = pc::MakeHash<pg::VertexVshData>(outPath.GetFullPathNoExt());
    refs.push_back(nctx.fullPath.data() + nctx.delimiter);
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
    indicesHash = pc::MakeHash<pg::VertexIndexData>(outPath.GetFullPathNoExt());
    refs.push_back(nctx.fullPath.data() + nctx.delimiter);
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
    pixelHash = pc::MakeHash<pg::VertexPshData>(outPath.GetFullPathNoExt());
    refs.push_back(nctx.fullPath.data() + nctx.delimiter);
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
  auto vayHash = pc::MakeHash<pg::VertexArray>(outPath.GetFullPathNoExt());
  {
    outFile =
        outPath.ChangeExtension2(pc::GetClassExtension<pg::VertexArray>());
    refs.push_back(outFile);
    flatbuffers::FlatBufferBuilder builder(1024);
    std::vector<flatbuffers::Offset<pg::VertexBuffer>> buffers;

    {
      std::vector<pg::VertexAttribute> attrs;
      attrs.emplace_back(pg::VertexType::Position, 3, GL_UNSIGNED_BYTE, false,
                         0);
      buffers.emplace_back(pg::CreateVertexBuffer(
          builder, builder.CreateVectorOfStructs(attrs), &posHash,
          4 * oVertices.size(), GL_ARRAY_BUFFER, GL_STATIC_DRAW, 4));
    }

    {
      std::vector<pg::VertexAttribute> attrs;
      attrs.emplace_back(pg::VertexType::Tangent, 4, GL_SHORT, true, 0);
      attrs.emplace_back(pg::VertexType::TexCoord2, 2, GL_UNSIGNED_SHORT, false,
                         8);
      buffers.emplace_back(pg::CreateVertexBuffer(
          builder, builder.CreateVectorOfStructs(attrs), &pixelHash,
          12 * oVertices.size(), GL_ARRAY_BUFFER, GL_STATIC_DRAW, 12));
    }

    buffers.emplace_back(
        pg::CreateVertexBuffer(builder, 0, &indicesHash, 2 * indices.size(),
                               GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, 0));

    auto buffersPtr = builder.CreateVector(buffers);

    std::vector<pc::Transform> transforms;
    transforms.emplace_back(pc::Transform{
        .tm =
            {
                {1, 0, 0, 0},
                {frame->offset.x, frame->offset.y, frame->offset.z},
            },
        .inflate = {frame->scale.x, frame->scale.y, frame->scale.z},
    });

    auto tmsPtr = builder.CreateVectorOfStructs(transforms);

    std::vector<glm::vec4> uvTransforms;
    uvMax -= uvMin;
    uvMax *= 1.f / 0xffff;
    uvTransforms.emplace_back(uvMin.x, uvMin.y, uvMax.x, uvMax.y);

    auto uvtmsPtr = builder.CreateVectorOfStructs(uvTransforms);
    const int8_t uvRemaps[]{0};
    auto uvRemapsPtr = builder.CreateVector(uvRemaps, sizeof(uvRemaps));

    pg::VertexArrayBuilder varBuild(builder);
    varBuild.add_buffers(buffersPtr);
    varBuild.add_transforms(tmsPtr);
    varBuild.add_uvTransforms(uvtmsPtr);
    varBuild.add_count(indices.size());
    varBuild.add_mode(GL_TRIANGLES);
    varBuild.add_type(GL_UNSIGNED_SHORT);
    varBuild.add_uvTransformRemaps(uvRemapsPtr);
    varBuild.add_index(-1);
    varBuild.add_tsType(pg::TSType::QTangent);
    pc::AABB aabb_{
        .center = (uData.max + uData.min) / 2,
        .bounds = uData.max,
    };
    aabb_.bounds -= aabb_.center;
    aabb_.bounds.w =
        std::max(std::max(aabb_.bounds.x, aabb_.bounds.y), aabb_.bounds.z);

    varBuild.add_aabb(&aabb_);

    prime::utils::FinishFlatBuffer(varBuild);

    BinWritterRef wrh(ctx->NewFile(outFile).str);
    wrh.WriteBuffer(reinterpret_cast<char *>(builder.GetBufferPointer()),
                    builder.GetSize());
  }

  // Create single model
  {
    flatbuffers::FlatBufferBuilder builder(1024);

    AFileInfo textureFile(hdr.skins->path);
    AFileInfo newBranch(textureFile.CatchBranch(outFile));

    static const auto extD = pc::GetClassExtension<pg::TextureStream<0>>();
    static std::string_view ext(extD);

    struct TextureSlot {
      uint32 hash;
      std::string_view slot;
    };

    std::vector<TextureSlot> textures;

    auto AddStreams = [ctx, &refs, &outDir](std::string &mainPath) {
      size_t numStreams = 4;

      try {
        auto texStream = ctx->RequestFile(outDir + mainPath);
        BinReaderRef txrd(*texStream.Get());
        std::string txBuffer;
        txrd.ReadContainer(txBuffer, txrd.GetSize());
        auto txHdr = prime::utils::GetFlatbuffer<pg::Texture>(txBuffer);
        numStreams = txHdr->info()->numStreams();
      } catch (...) {
      }

      for (size_t s = 0; s < numStreams; s++) {
        std::string texStream(AFileInfo(mainPath).GetFullPathNoExt());
        texStream.push_back('.');
        texStream.append(ext.substr(0, 3));
        texStream.push_back('0' + s);
        refs.push_back(texStream);
      }
    };

    static const auto texExt = pc::GetClassExtension<pg::Texture>();

    {
      uint32 texHash = JenkinsHash3_(newBranch.GetFullPathNoExt());
      auto outTexture = newBranch.ChangeExtension2(texExt);
      refs.push_back(outTexture);
      AddStreams(outTexture);
      textures.emplace_back(TextureSlot{texHash, "smAlbedo"});
    }

    std::vector<std::string_view> defines({
        /**/ //
        "NUM_LIGHTS=6",
    });

    try {
      ctx->FindFile(outDir + std::string(newBranch.GetFolder()),
                    "^" + std::string(newBranch.GetFilename()) + "_normal." +
                        std::string(texExt) + "$");
      std::string normalTexture(
          newBranch.ChangeExtension("_normal." + std::string(texExt)));

      uint32 texHash = JenkinsHash3_(newBranch.ChangeExtension("_normal"));
      refs.push_back(normalTexture);
      AddStreams(normalTexture);
      textures.emplace_back(TextureSlot{texHash, "smNormal"});
      defines.emplace_back("PS_IN_NORMAL");
    } catch (const es::FileNotFoundError &) {
    }

    try {
      ctx->FindFile(outDir + std::string(newBranch.GetFolder()),
                    "^" + std::string(newBranch.GetFilename()) + "_fx." +
                        std::string(texExt) + "$");
      std::string glowTexture(
          newBranch.ChangeExtension("_fx." + std::string(texExt)));

      uint32 texHash = JenkinsHash3_(newBranch.ChangeExtension("_fx"));
      refs.push_back(glowTexture);
      AddStreams(glowTexture);
      textures.emplace_back(TextureSlot{texHash, "smGlow"});
      defines.emplace_back("PS_IN_GLOW");
    } catch (const es::FileNotFoundError &) {
    }

    auto definesPtr = [&] {
      std::vector<flatbuffers::Offset<flatbuffers::String>> offsets;
      for (auto s : defines) {
        offsets.emplace_back(builder.CreateString(s));
      }
      return builder.CreateVector(offsets);
    }();

    auto stagesPtr = [&] {
      std::vector<pg::StageObject> objects;
      objects.emplace_back(JenkinsHash3_("simple_model/main.vert"), 0,
                           GL_VERTEX_SHADER);
      objects.emplace_back(JenkinsHash3_("simple_model/main.frag"), 0,
                           GL_FRAGMENT_SHADER);
      return builder.CreateVectorOfStructs(objects);
    }();

    pg::ProgramBuilder pgmBuilder(builder);
    pgmBuilder.add_definitions(definesPtr);
    pgmBuilder.add_stages(stagesPtr);
    pgmBuilder.add_program(-1);
    auto pgmPtr = pgmBuilder.Finish();

    auto texturesPtr = [&] {
      std::vector<pg::SampledTexture> textures_;

      for (auto &t : textures) {
        textures_.emplace_back(t.hash, JenkinsHash_(t.slot),
                               JenkinsHash3_("res/default"), 0, 0);
      }

      return builder.CreateVectorOfStructs(textures_);
    }();

    auto uniBlocksPtr = [&] {
      std::vector<pg::UniformBlock> ublocks;
      ublocks.emplace_back(pc::MakeHash<pg::UniformBlockData>("main_uniform"),
                           JenkinsHash_("ubFragmentProperties"), 0, 0, 0);

      return builder.CreateVectorOfStructs(ublocks);
    }();

    auto vayPtr = builder.CreateStruct(vayHash);
    pg::ModelRuntime runtimeDummy{};

    pg::ModelSingleBuilder mdlBuilder(builder);
    mdlBuilder.add_program(pgmPtr);
    mdlBuilder.add_textures(texturesPtr);
    mdlBuilder.add_uniformBlocks(uniBlocksPtr);
    mdlBuilder.add_runtime(&runtimeDummy);
    mdlBuilder.add_vertexArray_type(pc::ResourceVar::ResourceVar_hash);
    mdlBuilder.add_vertexArray(vayPtr.Union());

    prime::utils::FinishFlatBuffer(mdlBuilder);

    outFile = ctx->workingFile.ChangeExtension2(
        pc::GetClassExtension<pg::ModelSingle>());
    BinWritterRef wrh(ctx->NewFile(outFile).str);
    wrh.WriteBuffer(reinterpret_cast<char *>(builder.GetBufferPointer()),
                    builder.GetSize());
  }

  outFile = ctx->workingFile.ChangeExtension(".refs");
  BinWritterRef refWr(ctx->NewFile(outFile).str);

  for (auto &ref : refs) {
    refWr.WriteContainer(ref);
    refWr.Write('\n');
  }
}

void AppProcessFile(AppContext *ctx) {
  BinReaderRef rd(ctx->GetStream());
  uint32 id;
  rd.Read(id);
  rd.Seek(0);

  if (id == MD2::Header::ID) {
    try {
      ctx->RequestFile(std::string(ctx->workingFile.ChangeExtension(".iqm")));
      printwarning("IQM vaiant found, skipping MD2 processing");
    } catch (const es::FileNotFoundError &e) {
      std::string buffer;
      rd.ReadContainer(buffer, rd.GetSize());
      Process(*reinterpret_cast<MD2::Header *>(buffer.data()), ctx);
    }
  } else {
    throw es::InvalidHeaderError(id);
  }
}
