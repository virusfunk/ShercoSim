cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(dual-readout)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HEPMC_DIR    "$ENV{HEPMC_DIR}")
set(PYTHIA_DIR   "$ENV{PYTHIA_DIR}")
set(FASTJET_DIR  "$ENV{FASTJET_DIR}")

add_subdirectory(rootIO)
add_subdirectory(DRsim)

# Gen requires Pythia8 — skip gracefully if not present
find_package(Pythia8 QUIET)
if(Pythia8_FOUND)
  add_subdirectory(Gen)
else()
  message(STATUS "Pythia8 not found — skipping Gen")
endif()

# Reco and analysis are optional C++ tools; the tutorial uses Python analysis
# add_subdirectory(Reco)
# add_subdirectory(analysis)
