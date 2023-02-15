
namespace prime::utils {

template <class C> struct Builder {
  std::string buffer;

  Builder() {
    buffer.reserve(4096);
    buffer.resize(sizeof(C));
    std::construct_at(&Data());
  }

  C &Data() { return *reinterpret_cast<C *>(buffer.data()); }

  template <class A> void SetArray(common::Array<A> &array, size_t numItems) {
    const size_t dataBegin = buffer.size();
    array.numItems = numItems;
    const size_t arrayOffset = reinterpret_cast<size_t>(&array) -
                               reinterpret_cast<size_t>(buffer.data());
    array.pointer = dataBegin - arrayOffset;

    buffer.resize(dataBegin + sizeof(A) * numItems);
  }

  template <class A, class B>
  void SetArray(common::LocalArray<A, B> &array, size_t numItems) {
    const size_t dataBegin = buffer.size();
    array.numItems = numItems;
    const size_t arrayOffset = reinterpret_cast<size_t>(&array.pointer) -
                               reinterpret_cast<size_t>(buffer.data());
    array.pointer = dataBegin - arrayOffset;

    buffer.resize(dataBegin + sizeof(A) * numItems);
  }

  template <class A> common::Array<A> ArrayData(common::Array<A> &array) {
    common::Array<A> newArray = array;
    uint64 addr = reinterpret_cast<uint64>(&array);
    newArray.pointer = addr + array.pointer;
    return newArray;
  }
};

struct DefineBuilder {
  std::string buffer;

  void AddDefine(std::string_view data) {
    buffer.push_back(data.size());
    buffer.append(data);
  }

template<class C>
  void AddKeyValue(std::string_view data, C value) {
    auto valueStr = std::to_string(value);
    buffer.push_back(data.size() + 1 + valueStr.size());
    buffer.append(data);
    buffer.push_back('=');
    buffer.append(valueStr);
  }
};
} // namespace prime::utils
