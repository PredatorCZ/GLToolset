add_subdirectory(${TPD_PATH}/veverka ${CMAKE_CURRENT_BINARY_DIR}/veverka)

build_target(
  NAME
  prime_script
  TYPE
  OBJECT
  SOURCES
  funcproto.cpp
  sqbase/sqvm.cpp
  sqbase/sqapi.cpp
  sqbase/sqclass.cpp
  sqbase/sqbaselib.cpp
  sqbase/sqdebug.cpp
  sqbase/sqobject.cpp
  sqbase/sqstate.cpp
  sqbase/sqfuncstate.cpp
  sqbase/sqcompiler.cpp
  sqbase/sqtable.cpp
  sqbase/sqlexer.cpp
  sqbase/sqmem.cpp
  LINKS
  squirrel_static
  spike-interface
)
