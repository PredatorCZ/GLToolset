#include "utils/playground.hpp"
#include <algorithm>

#include <cassert>

namespace prime::utils {

std::string PlayGround::Build() {
  std::string retval;
  if (blocks.empty()) {
    return retval;
  }

  retval.append(blocks.front().View(this));

  common::ResourceBase &res =
      reinterpret_cast<common::ResourceBase &>(retval.front());
  res.maxAlign = blocks.front().alignment;

  if (blocks.size() == 1) {
    return retval;
  }

  std::vector<Block> sortedBlocks(std::next(blocks.begin()), blocks.end());
  std::sort(sortedBlocks.begin(), sortedBlocks.end(),
            [](const Block &g0, const Block &g1) {
              return g0.alignment > g1.alignment;
            });

  std::vector<Block> newBlocks{blocks.front()};
  res.maxAlign = std::max(uint32(res.maxAlign), sortedBlocks.front().alignment);

  for (auto &g : sortedBlocks) {
    const uint32 padding = GetPadding(retval.size(), g.alignment);
    // TODO reuse padding
    retval.append(padding, 0);

    std::string_view block(g.View(this));
    newBlocks.emplace_back(Block{
        .begin = uint32(retval.size()),
        .end = uint32(retval.size() + block.size()),
        .alignment = g.alignment,
    });
    retval.append(block);
  }

  sortedBlocks.insert(sortedBlocks.begin(), blocks.front());

  for (LocalPointer &p : localPointers) {
    for (size_t i = 0; i < sortedBlocks.size(); i++) {
      Block &oldBlock = sortedBlocks[i];
      Block &newBlock = newBlocks[i];

      if (oldBlock.begin <= p.offset && p.offset < oldBlock.end) {
        const uint32 offsetFromBlock = p.offset - oldBlock.begin;

        auto found = std::find_if(
            sortedBlocks.begin(), sortedBlocks.end(), [&p](Block &b) {
              return b.begin <= p.pointsTo && p.pointsTo < b.end;
            });

        uint32 newPoint =
            newBlocks.at(std::distance(sortedBlocks.begin(), found)).begin;

        if (p.size == 2) {
          int16 *ptr = reinterpret_cast<int16 *>(
              retval.data() + newBlock.begin + offsetFromBlock);
          *ptr = Relative(retval.at(newPoint), *ptr);
        } else if (p.size == 4) {
          int32 *ptr = reinterpret_cast<int32 *>(
              retval.data() + newBlock.begin + offsetFromBlock);
          *ptr = Relative(retval.at(newPoint), *ptr);
        }
      }
    }
  }

  return retval;
}

PlayGround::BtPointer PlayGround::Allocate(uint32 size, uint32 alignment) {
  Gap *endGap = nullptr;

  for (auto &g : gaps) {
    const uint32 startPadding = GetPadding(g.begin, alignment);
    const uint32 alignedBegin = g.begin + startPadding;

    if (g.end == data.size()) {
      endGap = &g;
    }

    if (alignedBegin >= g.end) {
      continue;
    }

    if (g.end - alignedBegin >= size) {
      if (g.end - alignedBegin == size) {
        if (startPadding) {
          g.end = alignedBegin;
          SortGaps();
        } else {
          // printf("removing gap [%d %d]\n", g.begin, g.end);
          std::erase(gaps, g);
        }
      } else {
        uint32 oldBegin = g.begin;
        g.begin = alignedBegin + size;
        // printf("change gap [%d %d] -> [%d %d]\n", oldBegin, g.end, g.begin,
        // g.end);
        if (startPadding) {
          // printf("add gap [%d %d]\n", oldBegin, alignedBegin);
          gaps.emplace_back(Gap{oldBegin, alignedBegin});
        }
        SortGaps();
      }

      // printf("add block [%d %d]\n", alignedBegin, alignedBegin + size);
      blocks.emplace_back(Block{alignedBegin, alignedBegin + size, alignment});
      return {alignedBegin, this};
    }
  }

  if (endGap) {
    const uint32 gapSize = endGap->end - endGap->begin;
    const uint32 startPadding = GetPadding(endGap->begin, alignment);
    const uint32 alignedBegin = endGap->begin + startPadding;

    if (startPadding) {
      endGap->end = alignedBegin;
      SortGaps();
    } else {
      gaps.erase(std::next(gaps.begin(), std::distance(gaps.data(), endGap)));
    }

    data.append(std::max(uint32(0x2000), size + startPadding) - gapSize, 0);
    blocks.emplace_back(Block{alignedBegin, alignedBegin + size, alignment});

    return {alignedBegin, this};
  }

  const uint32 dataSize = data.size();
  const uint32 startPadding = GetPadding(dataSize, alignment);
  const uint32 alignedBegin = dataSize + startPadding;
  if (startPadding) {
    gaps.emplace_back(Gap{dataSize, alignedBegin});
    SortGaps();
  }
  data.append(std::max(uint32(0x2000), size + startPadding), 0);
  if (alignedBegin + size != data.size()) {
    gaps.emplace_back(Gap{alignedBegin + size, uint32(data.size())});
  }
  blocks.emplace_back(Block{alignedBegin, alignedBegin + size, alignment});

  return {alignedBegin, this};
}

PlayGround::BtPointer PlayGround::Realloc(uint32 oldOffset, uint32 newSize,
                                          uint32 itemAlignment) {
  auto found =
      std::find_if(blocks.begin(), blocks.end(),
                   [&](const Block &item) { return item.begin == oldOffset; });

  assert(found != blocks.end());

  auto foundGap = std::find_if(gaps.begin(), gaps.end(), [&](const Gap &gap) {
    return gap.begin == found->end;
  });

  const uint32 oldSize = found->end - found->begin;

  if (foundGap != gaps.end()) {
    const uint32 gapSize = foundGap->end - foundGap->begin;
    const uint32 deltaSize = newSize - oldSize;

    if (gapSize + oldSize >= newSize) {
      if (gapSize + oldSize == newSize) {
        // printf("removing gap [%d %d]\n", foundGap->begin, foundGap->end);
        std::erase(gaps, *foundGap);
      } else {
        // printf("change gap [%d %d] -> [%d %d]\n", foundGap->begin,
        //       foundGap->end, foundGap->begin + deltaSize, foundGap->end);
        foundGap->begin += deltaSize;
        SortGaps();
      }

      // printf("change block [%d %d] -> [%d %d]\n", found->begin, found->end,
      //        found->begin, found->end + deltaSize);
      found->end += deltaSize;

      return {oldOffset, this};
    }
  }

  if (found->end == data.size()) {
    data.append(0x2000, 0);
    found->end = found->begin + newSize;
    gaps.emplace_back(Gap{found->end, uint32(data.size())});
    return {oldOffset, this};
  }

  Block thisBlock = *found;
  std::erase(blocks, *found);

  BtPointer newData = Allocate(newSize, itemAlignment);
  assert(newData.offset + newSize <= data.size());
  memcpy(data.data() + newData.offset, data.data() + thisBlock.begin, oldSize);
  NotifyDataMove(oldOffset, newData.offset, oldSize);

  // printf("block to gap [%d %d]\n", found->begin, found->end);
  gaps.emplace_back(Gap{thisBlock.begin, thisBlock.end});
  SortGaps();
  return newData;
}

void PlayGround::SortGaps() {
  std::sort(gaps.begin(), gaps.end(),
            [](const Gap &g0, const Gap &g1) { return g0.begin < g1.begin; });
  std::vector<Gap> oldGaps = std::move(gaps);
  gaps.emplace_back(oldGaps.front());
  oldGaps.erase(oldGaps.begin());

  for (auto &g : oldGaps) {
    if (g.begin == gaps.back().end) {
      gaps.back().end = g.end;
    } else {
      gaps.emplace_back(g);
    }
  }

  std::sort(gaps.begin(), gaps.end(), [](const Gap &g0, const Gap &g1) {
    return (g0.end - g0.begin) < (g1.end - g1.begin);
  });
}

void PlayGround::NotifyDataMove(uint32 oldLoc, uint32 newLoc,
                                uint32 oldLocSize) {
  for (BtPointer *p : pointers) {
    if (p->offset >= oldLoc && p->offset < oldLoc + oldLocSize) {
      const uint32 deltaOffset = p->offset - oldLoc;
      p->offset = newLoc + deltaOffset;
    }
  }

  for (LocalPointer &p : localPointers) {
    if (p.offset >= oldLoc && p.offset < oldLoc + oldLocSize) {
      if (p.size == 2) {
        int16 *ptr = reinterpret_cast<int16 *>(data.data() + p.offset);
        char *pointsTo = reinterpret_cast<char *>(ptr) + *ptr;
        p.offset = p.offset - oldLoc + newLoc;
        int16 *nPtr = reinterpret_cast<int16 *>(data.data() + p.offset);
        *nPtr = Relative(*pointsTo, *nPtr);
      } else if (p.size == 4) {
        int32 *ptr = reinterpret_cast<int32 *>(data.data() + p.offset);
        char *pointsTo = reinterpret_cast<char *>(ptr) + *ptr;
        p.offset = p.offset - oldLoc + newLoc;
        int32 *nPtr = reinterpret_cast<int32 *>(data.data() + p.offset);
        *nPtr = Relative(*pointsTo, *nPtr);
      }
    }
  }
  for (LocalPointer &p : localPointers) {
    if (p.pointsTo >= oldLoc && p.pointsTo < oldLoc + oldLocSize) {
      const uint32 oldPoint = p.pointsTo;
      p.pointsTo = oldPoint - oldLoc + newLoc;

      // printf("pointer %d -> %d newly points %d\n", p.offset, oldPoint,
      //        p.pointsTo);

      if (p.size == 2) {
        int16 *ptr = reinterpret_cast<int16 *>(data.data() + p.offset);
        *ptr = Relative(data.at(p.pointsTo), *ptr);
      } else if (p.size == 4) {
        int32 *ptr = reinterpret_cast<int32 *>(data.data() + p.offset);
        *ptr = Relative(data.at(p.pointsTo), *ptr);
      }
    }
  }
}

void PlayGround::AddPointer(int32 *ptr) {
  const uint32 offset = Relative(*ptr, data.front());
  const uint32 pointsTo =
      Relative(*(reinterpret_cast<const char *>(ptr) + *ptr), data.front());

  // printf("new pointer: %d -> %d\n", offset, pointsTo);

  auto found =
      std::find_if(localPointers.begin(), localPointers.end(),
                   [offset](LocalPointer &p) { return offset == p.offset; });

  if (found != localPointers.end()) {
    found->pointsTo = pointsTo;
  } else {
    localPointers.emplace_back(
        LocalPointer{.pointsTo = pointsTo, .offset = offset, .size = 4});
  }
}

void PlayGround::NewString(common::String &arrayRef, std::string_view value) {
  arrayRef.asBig.length = value.size() << 1;

  if (value.size() > 7) {
      Pointer<char> ptr = Allocate(value.size(), 1);
      memcpy(ptr.operator->(), value.data(), value.size());
      arrayRef.asBig.data = ptr.operator->();
      AddPointer(&arrayRef.asBig.data.pointer);
      return;
    }

    arrayRef.asTiny.length |= 1;
    memcpy(arrayRef.asTiny.data , value.data(), value.size());
}


} // namespace prime::utils
