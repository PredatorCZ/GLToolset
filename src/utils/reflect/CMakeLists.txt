build_target(
  NAME
  prime_reflect
  TYPE
  OBJECT
  SOURCES
  graphics.cpp
  reflect.cpp
  script.cpp
  utils.cpp
  LINKS
  spike-interface
  PROPERTIES
  POSITION_INDEPENDENT_CODE
  ON
)
