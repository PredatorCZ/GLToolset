#include "graphics/model_single.hpp"
#include "spike/util/unit_testing.hpp"
#include "utils/debug.hpp"

namespace pu = prime::utils;
namespace pc = prime::common;

struct SubClass {
  pc::String name;
  uint32 number;
};

HASH_CLASS(SubClass);

struct SampleClass {
  uint32 number;
  float real;
  pc::LocalArray32<SubClass> subClasses;
  pc::LocalArray16<pc::String> names;
};

HASH_CLASS(SampleClass);

int main() {
  es::print::AddPrinterFunction(es::Print);
  pu::PlayGround pg;
  pu::PlayGround::Pointer<SampleClass> newClass = pg.AddClass<SampleClass>();

  newClass->number = 10;
  newClass->real = 5.89f;

  pu::PlayGround::Pointer<SubClass> newSubClass =
      pg.ArrayEmplace(newClass->subClasses);
  pg.NewString(*pg.ArrayEmplace(newClass->names), "name0");
  pg.NewString(newSubClass->name, "sample name 1");
  pg.NewString(*pg.ArrayEmplace(newClass->names), "name1");
  newSubClass->number = 158;
  pu::PlayGround::Pointer<SubClass> newSubClass2 =
      pg.ArrayEmplace(newClass->subClasses);
  pg.NewString(newSubClass2->name, "sample name 2");
  newSubClass2->number = 157;
  pg.NewString(*pg.ArrayEmplace(newClass->names), "name2");

  TEST_EQUAL(newClass->number, 10);
  TEST_EQUAL(newClass->real, 5.89f);
  TEST_EQUAL(newClass->subClasses.numItems, 2);
  TEST_EQUAL(newClass->names.numItems, 3);

  {
    SubClass &sc0 = newClass->subClasses[0];
    auto &sc0Name = sc0.name;
    const char *name0Data = sc0Name.raw();

    SubClass &sc1 = newClass->subClasses[1];
    auto &sc1Name = sc1.name;
    const char *name1Data = sc1Name.raw();

    TEST_EQUAL(sc0.number, 158);
    TEST_EQUAL(memcmp(name0Data, "sample name 1", 13), 0);

    TEST_EQUAL(sc1.number, 157);
    TEST_EQUAL(memcmp(name1Data, "sample name 2", 13), 0);

    TEST_EQUAL(memcmp(newClass->names[0].raw(), "name0", 5), 0);
    TEST_EQUAL(memcmp(newClass->names[1].raw(), "name1", 5), 0);
    TEST_EQUAL(memcmp(newClass->names[2].raw(), "name2", 5), 0);
  }

  std::string built = pg.Build();
  SampleClass *cls = reinterpret_cast<SampleClass *>(built.data());

  {
    SubClass &sc0 = cls->subClasses[0];
    auto &sc0Name = sc0.name;
    const char *name0Data = sc0Name.raw();

    SubClass &sc1 = cls->subClasses[1];
    auto &sc1Name = sc1.name;
    const char *name1Data = sc1Name.raw();

    TEST_EQUAL(sc0.number, 158);
    TEST_EQUAL(memcmp(name0Data, "sample name 1", 13), 0);

    TEST_EQUAL(sc1.number, 157);
    TEST_EQUAL(memcmp(name1Data, "sample name 2", 13), 0);

    TEST_EQUAL(memcmp(cls->names[0].raw(), "name0", 5), 0);
    TEST_EQUAL(memcmp(cls->names[1].raw(), "name1", 5), 0);
    TEST_EQUAL(memcmp(cls->names[2].raw(), "name2", 5), 0);
  }

  pu::ResourceDebugPlayground debugPg;
  pu::PlayGround pgn;
  std::string debug = debugPg.Build<prime::graphics::ModelSingle>(pgn);

  pu::ResourceDebugFooter *footer = reinterpret_cast<pu::ResourceDebugFooter *>(
      debug.data() + debug.size() - sizeof(pu::ResourceDebugFooter));

  pu::ResourceDebug *hdr =
      reinterpret_cast<pu::ResourceDebug *>(debug.data() + footer->dataSize);
  pu::ResourceDebugClass *mainCls = hdr->classes.begin();
  const prime::reflect::Class *rfClass =
      prime::reflect::GetReflectedClass<prime::graphics::ModelSingle>();

  TEST_EQUAL(memcmp(mainCls->className.raw(), rfClass->className,
                    strlen(rfClass->className)),
             0);
  TEST_EQUAL(mainCls->members.numItems, rfClass->nMembers);

  for (uint32 i = 0; i < rfClass->nMembers; i++) {
    auto &rfMember = rfClass->members[i];
    auto &member = mainCls->members[i];
    std::string_view memberName(member.name);

    TEST_EQUAL(memcmp(member.name.raw(), rfMember.name, strlen(rfMember.name)),
               0);
    TEST_EQUAL(member.offset, rfMember.offset);
    TEST_EQUAL(member.types.numItems, rfMember.nTypes);

    for (uint32 j = 0; auto &type : member.types) {
      auto &rfType = rfMember.types[j++];

      TEST_EQUAL(type.alignment, rfType.alignment);
      TEST_EQUAL(type.container, rfType.container);
      TEST_EQUAL(type.count, rfType.count);
      TEST_EQUAL(type.size, rfType.size);
      TEST_EQUAL(type.type, rfType.type);

      if (type.type == prime::reflect::Type::Class) {
        TEST_CHECK(type.definition.operator bool());
        bool found = false;
        const pu::ResourceDebugClass *defClass = type.definition;

        for (auto &c : hdr->classes) {
          if (&c == defClass) {
            found = true;
            std::string_view def(defClass->className);
            TEST_EQUAL(memcmp(c.className.raw(), def.data(), def.size()), 0);
            break;
          }
        }

        TEST_CHECK(found);
      }
    }
  }

  return 0;
}
