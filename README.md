# DRC cosmic : 27 by 27 geometry
Repository for GEANT4 simulation &amp; analysis of the dual-readout calorimeter for general purpose.
This package is executable on CentOS7 with cvmfs.

## How-to
### Compile
    
    cd install
    source envset.sh
    cd ../build
    
    # option 1. with visualization
    cmake -DWITH_GEANT4_UIVIS=ON -DCMAKE_INSTALL_PREFIX=../install ..
    # option 2. without visualization
    cmake ../ -DCMAKE_INSTALL_PREFIX=../install
    
    make -j4 install

### Simulation

    In [install] directory,
    ./bin/DRsim <macro_file> <seed> <outputfile_name>
    
### Analysis
    
    In [install] directory,
    ./bin/analysis <path_to_root_files> <low_edge_of_hist> <high_edge_of_hist> <do_calib>

e.g.)

    ./bin/analysis ./plot/20GeV_ele 0 20 0

### Visualization

After compiling, inside /install directory, run

    ./bin/DRsim
