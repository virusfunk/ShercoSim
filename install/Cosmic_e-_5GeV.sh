#! /bin/sh

source /cvmfs/sft.cern.ch/lcg/views/LCG_108/x86_64-el9-gcc14-opt/setup.sh
source /cvmfs/sft.cern.ch/lcg/releases/LCG_108/ROOT/6.36.02/x86_64-el9-gcc14-opt/ROOT-env.sh
source /cvmfs/geant4.cern.ch/geant4/11.3.p02/x86_64-el9-gcc14-optdeb-MT/CMake-setup.sh

./bin/DRsim Cosmic_e-_5GeV.mac $1 /your/path/root/Cosmic_e-_5GeV
