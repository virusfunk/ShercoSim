project(rootIO)

find_package(ROOT REQUIRED)

# FastJet is optional — omit fastjetInterface when not available
set(FASTJET_LIB "")
set(FASTJET_INCLUDE "")
if(DEFINED FASTJET_DIR AND NOT "${FASTJET_DIR}" STREQUAL "")
  find_library(FASTJET_FOUND NAMES fastjet PATHS "${FASTJET_DIR}/lib")
endif()

if(FASTJET_FOUND)
  message(STATUS "FastJet found — enabling fastjetInterface")
  set(FASTJET_LIB "${FASTJET_DIR}/lib/libfastjet.so")
  set(FASTJET_INCLUDE "${FASTJET_DIR}/include")
  set(LINKDEF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/include/LinkDef.h)
  file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
  file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*Interface.h)
else()
  message(STATUS "FastJet not found — disabling fastjetInterface")
  set(LINKDEF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/include/LinkDef_nofastjet.h)
  file(GLOB _all_sources ${PROJECT_SOURCE_DIR}/src/*.cc)
  foreach(_src ${_all_sources})
    if(NOT _src MATCHES "fastjetInterface")
      list(APPEND sources ${_src})
    endif()
  endforeach()
  file(GLOB _all_headers ${PROJECT_SOURCE_DIR}/include/*Interface.h)
  foreach(_hdr ${_all_headers})
    if(NOT _hdr MATCHES "fastjetInterface")
      list(APPEND headers ${_hdr})
    endif()
  endforeach()
endif()

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${FASTJET_INCLUDE}
)

include(${ROOT_USE_FILE})
ROOT_GENERATE_DICTIONARY(G__rootIO ${headers} LINKDEF ${LINKDEF_FILE})
add_library(rootIO SHARED ${sources} G__rootIO.cxx)
target_include_directories(rootIO PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${ROOT_INCLUDE_DIRS}
  ${FASTJET_INCLUDE}
)
target_link_libraries(rootIO
  ${FASTJET_LIB}
  ${ROOT_LIBRARIES}
)

install(TARGETS rootIO LIBRARY DESTINATION lib)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/librootIO_rdict.pcm
  ${CMAKE_CURRENT_BINARY_DIR}/librootIO.rootmap
  DESTINATION lib
)
