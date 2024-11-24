#pragma once
#include "common/array.hpp"
#include "common/local_pointer.hpp"
#include "common/string.hpp"
#include "spike/util/supercore.hpp"
#include <algorithm>
#include <cassert>
#include <string>
#include <vector>
namespace prime::utils {

struct PlayGround {
  PlayGround() = default;
  PlayGround(const PlayGround &) = delete;
  PlayGround(PlayGround &&) = delete;

private:
  struct BtPointer {
    BtPointer(uint32 in, PlayGround *own) : offset(in), owner(own) {
      Register();
    }
    BtPointer(const BtPointer &o) : offset(o.offset), owner(o.owner) {
      Register();
    }
    BtPointer(BtPointer &&o) : offset(o.offset), owner(o.owner) {
      o.UnRegister();
      Register();
    }

    BtPointer &operator=(const BtPointer &) = delete;
    BtPointer &operator=(BtPointer &&) = delete;

    ~BtPointer() { UnRegister(); }
    void Register() {
      assert([&] {
        auto found =
            std::find(owner->pointers.begin(), owner->pointers.end(), this);
        return found == owner->pointers.end();
      }());
      owner->pointers.push_back(this);
    }
    void UnRegister() { std::erase(owner->pointers, this); }

    uint32 offset;
    PlayGround *owner;
  };

public:
  template <class C> struct Pointer : BtPointer {
    using BtPointer::BtPointer;

    Pointer(const BtPointer &o) : BtPointer(o) {}
    Pointer(BtPointer &&o) : BtPointer(std::move(o)) {}
    Pointer(const Pointer &o) : BtPointer(o) {}
    Pointer(Pointer &&o) : BtPointer(std::move(o)) {}
    // Pointer(const Pointer &) = default;
    // Pointer(Pointer &&) = default;

    Pointer &operator=(const Pointer &) = delete;
    Pointer &operator=(Pointer &&) = delete;

    C *operator->() {
      return reinterpret_cast<C *>(this->owner->data.data() + this->offset);
    }
    C &operator*() { return *operator->(); }
  };

  template <class C> Pointer<C> AddClass() {
    Pointer<C> retVal(Allocate(sizeof(C), alignof(C)));
    std::construct_at(retVal.operator->());
    return retVal;
  }

  template <class C, class D>
  void ArrayEmplace(common::LocalArray<C, D> &arrayRef, C item) {
    Pointer<common::LocalArray<C, D>> array(IncArray(arrayRef));
    (*array)[array->numItems++] = item;
  }

  template <class C, class D, class... Args>
  void ArrayEmplace(common::LocalArray<C, D> &arrayRef, Args... args) {
    Pointer<common::LocalArray<C, D>> array(IncArray(arrayRef));
    std::construct_at(&(*array)[array->numItems++], args...);
  }

  template <class C, class D>
  Pointer<C> ArrayEmplace(common::LocalArray<C, D> &arrayRef) {
    Pointer<common::LocalArray<C, D>> array(IncArray(arrayRef));
    C &item = (*array)[array->numItems++];
    std::construct_at(&item);
    return {uint32(Relative(item, data.front())), this};
  }

  template <class D>
  Pointer<char> ArrayEmplaceBytes(common::LocalArray<char, D> &arrayRef,
                                  uint32 size, uint32 alignment) {
    const uint32 numItems = arrayRef.numItems + 1;
    Pointer<common::LocalArray<char, D>> array(
        IncArray(arrayRef, size * numItems, size, alignment));
    char &item = (*array)[array->numItems++ * size];
    return {uint32(Relative(item, data.front())), this};
  }

  template <class C, class D, size_t N>
  void ArraySet(common::LocalArray<C, D> &arrayRef, const C (&value)[N]) {
    Pointer<common::LocalArray<C, D>> array(IncArray(arrayRef, sizeof(C) * N));
    array->numItems = N;
    memcpy(array->begin(), value, sizeof(C) * N);
  }

  template <size_t N>
  void NewString(common::String &arrayRef, const char (&value)[N]) {
    NewString(arrayRef, {value, N - 1});
  }

  void NewString(common::String &arrayRef, std::string_view value);

  template <class C>
  Pointer<C> NewBytes(const char *data, size_t size = sizeof(C)) {
    Pointer<C> retVal(Allocate(size, alignof(C)));
    memcpy(reinterpret_cast<void *>(retVal.operator->()), data, size);
    return retVal;
  }

  template <class C> void Link(common::LocalPointerBase<C> &ptrRef, C *item) {
    ptrRef = item;
    AddPointer(&ptrRef.pointer);
  }

  template <class... C, class D>
  void Link(common::LocalVariantPointer<C...> &ptrRef, D *item) {
    ptrRef = item;
    AddPointer(
        &reinterpret_cast<common::LocalPointerBase<char> &>(ptrRef).pointer);
  }

  std::string Build();

  template <class C> C *As() { return reinterpret_cast<C *>(data.data()); }

private:
  template <class C, class D>
  Pointer<common::LocalArray<C, D>>
  IncArray(common::LocalArray<C, D> &arrayRef, uint32 sizeOverride = 0,
           uint32 stride = sizeof(C), uint32 align = alignof(C)) {
    Pointer<common::LocalArray<C, D>> array(Relative(arrayRef, data.front()),
                                            this);
    const uint32 numItems = arrayRef.numItems;
    const uint32 newSize =
        sizeOverride ? sizeOverride : (numItems + 1) * stride;

    Pointer<C> ptr(numItems ? Realloc(Relative(*array->begin(), data.front()),
                                      newSize, align)
                            : Allocate(newSize, align));

    if (numItems == 0) {
      using ArrayType = common::LocalArray<C, D>;
      localPointers.emplace_back(LocalPointer{
          .pointsTo = ptr.offset,
          .offset = array.offset + uint32(offsetof(ArrayType, pointer)),
          .size = sizeof(D)});
    }
    array->pointer = Relative(*ptr, array->pointer);
    return array;
  }

  template <class A, class B> static int32 Relative(const A &a, const B &b) {
    const intptr_t aadr = reinterpret_cast<intptr_t>(&a);
    const intptr_t badr = reinterpret_cast<intptr_t>(&b);
    return aadr - badr;
  }
  struct Gap {
    uint32 begin;
    uint32 end;

    bool operator==(const Gap &other) const {
      return (begin == other.begin) && (end == other.end);
    }
  };

  struct LocalPointer {
    uint32 pointsTo;
    uint32 offset;
    uint32 size;
  };

  struct Block {
    uint32 begin;
    uint32 end;
    uint32 alignment;

    std::string_view View(PlayGround *pg) {
      return {pg->data.data() + begin, pg->data.data() + end};
    }

    bool operator==(const Block &other) const {
      return begin == other.begin && end == other.end;
    }
  };

  BtPointer Allocate(uint32 size, uint32 alignment);
  BtPointer Realloc(uint32 oldOffset, uint32 itemSize, uint32 itemAlignment);
  void SortGaps();
  void NotifyDataMove(uint32 oldLoc, uint32 newLoc, uint32 oldLocSize);
  void AddPointer(int32 *ptr);

  std::vector<Block> blocks;
  std::vector<Gap> gaps{Gap{0, 0x2000}};
  std::string data = std::string(size_t(0x2000), '\0');
  std::vector<BtPointer *> pointers;
  std::vector<LocalPointer> localPointers;
};
} // namespace prime::utils
