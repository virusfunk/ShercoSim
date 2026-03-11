# DRC cosmic : 27 by 27 geometry
Repository for GEANT4 simulation &amp; analysis of the dual-readout calorimeter for general purpose.

This package is running at Alma9 with GEANT4 11.3.2.

## How-to
### Compile
After fetching the repository (Alma9, Geant4 11.3.2 installed), do

    cd build

    # option 1. with visualization
    cmake -DWITH_GEANT4_UIVIS=ON -DCMAKE_INSTALL_PREFIX=../install ..
    # option 2. without visualization
    cmake ../ -DCMAKE_INSTALL_PREFIX=../install

    make -j4 install

If you use cvmfs for environment, do
    
    cd install
    source envset.sh
    cd ../build

    # option 1. with visualization
    cmake -DWITH_GEANT4_UIVIS=ON -DCMAKE_INSTALL_PREFIX=../install ..
    # option 2. without visualization
    cmake ../ -DCMAKE_INSTALL_PREFIX=../install
    
    make -j4 install

### Run

    cd ../install
    ./bin/DRsim <macro_file> <seed> <outputfile_name>

e.g.)

    ./bin/DRsim run_ele.mac 1 output

### Analysis

    ./bin/analysis <path_to_root_files> <low_edge_of_hist> <high_edge_of_hist> <do_calib>

e.g.)

    ./bin/analysis ./plot/20GeV_ele 0 20 0

### Visualization

After compiling, inside /install directory, run

    ./bin/DRsim
