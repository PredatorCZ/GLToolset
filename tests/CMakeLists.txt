
build_target(
  NAME
  playground
  TYPE
  APP
  SOURCES
  test_playground.cpp
  ../src/utils/playground.cpp
  ../src/utils/debug.cpp
  ../src/common/resource.cpp
  ../src/common/registry.cpp
  LINKS
  spike
  prime_reflect
  prime_script
  prime_converters
)
