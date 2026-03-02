#----------------------------------------------------------------------------
# Setup the project
project(DRsim)

#----------------------------------------------------------------------------
# Find Geant4 package (batch mode by default; set WITH_GEANT4_UIVIS=ON for GUI)
option(WITH_GEANT4_UIVIS "Build with Geant4 UI and Vis drivers" OFF)
if(WITH_GEANT4_UIVIS)
  find_package(Geant4 REQUIRED ui_all vis_all)
else()
  find_package(Geant4 REQUIRED)
endif()

include(${Geant4_USE_FILE})

#----------------------------------------------------------------------------
# HepMC3 — optional.  Without it the /DRsim/action/useHepMC command is a
# no-op (USE_HEPMC3 is not defined).
find_package(HepMC3 QUIET)
if(HepMC3_FOUND)
  message(STATUS "HepMC3 found — HepMC input enabled")
  add_definitions(-DUSE_HEPMC3)
  include_directories(${HepMC3_INCLUDE_DIR})
  set(HEPMC_LIBS ${HepMC3_LIBRARIES})
  # Also pick up the ROOT I/O library if present
  if(DEFINED HEPMC_DIR)
    list(APPEND HEPMC_LIBS
         "${HEPMC_DIR}/lib64/libHepMC3.so"
         "${HEPMC_DIR}/lib64/libHepMC3rootIO.so")
  endif()
else()
  message(STATUS "HepMC3 not found — CORSIKA/gun mode only")
  set(HEPMC_LIBS "")
endif()

#----------------------------------------------------------------------------
# Sources & headers
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${Geant4_INCLUDE_DIR}
)

file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

# DRsimTest.cc duplicates static member definitions — exclude it.
list(FILTER sources EXCLUDE REGEX "DRsimTest\\.cc$")

# If HepMC3 is absent, exclude the HepMC adapter sources to avoid link errors.
if(NOT HepMC3_FOUND)
  list(FILTER sources EXCLUDE REGEX "HepMCG4.*\\.cc$")
  list(FILTER headers EXCLUDE REGEX "HepMCG4.*\\.hh$")
endif()

#----------------------------------------------------------------------------
# Executable
add_executable(DRsim DRsim.cc ${sources} ${headers})
target_link_libraries(DRsim
  ${Geant4_LIBRARIES}
  ${HEPMC_LIBS}
  rootIO
)

#----------------------------------------------------------------------------
# Copy macro files to build directory
file(GLOB DRsim_MACROS ${PROJECT_SOURCE_DIR}/*.mac)
file(COPY ${DRsim_MACROS} DESTINATION ${PROJECT_BINARY_DIR})

install(TARGETS DRsim DESTINATION bin)
install(FILES ${DRsim_MACROS} DESTINATION .)
