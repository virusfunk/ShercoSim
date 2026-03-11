#!/bin/sh

# particle=("e-" "mu-" "pi-" "proton")
for i in "e-"
do
    run_beamOn="5"

    for j in 20 # energy [GeV]
    do
        beamAngle=("90,0")

        # index=0
        x="-100"
        y="0"
        z="0"

        for p in "${beamAngle[@]}"
        do
            theta="${p%%,*}"
            phi="${p##*,}"

            echo "Beam angle - theta: $theta, phi: $phi"

            macroname="Cosmic_${i}_${j}GeV"
                    
            gun_particle=$i
            gun_energy="$j GeV"
            root_name="${macroname}"

            ########################################################
            results="/your/path/$root_name"            
            ########################################################

            echo "starting submit for $gun_energy $gun_particle and the output will be $root_name.root"   
        
            echo "/DRsim/action/useHepMC False" >> $macroname.mac
            echo "/DRsim/action/useCalib False" >> $macroname.mac
            echo "/vis/disable" >> $macroname.mac
            echo "/run/numberOfThreads 1" >> $macroname.mac
            echo "/run/initialize" >>$macroname.mac
            echo "/run/verbose 1" >> $macroname.mac
            echo "/DRsim/generator/randx 0" >> $macroname.mac
            echo "/DRsim/generator/randy 10" >> $macroname.mac
            echo "/DRsim/generator/randz 10" >> $macroname.mac
            echo "/DRsim/generator/theta $theta" >> $macroname.mac
            echo "/DRsim/generator/phi $phi" >> $macroname.mac
            echo "/DRsim/generator/x0 $x" >> $macroname.mac
            echo "/DRsim/generator/y0 $y" >> $macroname.mac
            echo "/DRsim/generator/z0 $z" >> $macroname.mac
            echo "/gun/particle $i" >> $macroname.mac
            echo "/gun/energy $j GeV" >> $macroname.mac
            echo "/run/beamOn $run_beamOn" >> $macroname.mac

            echo "#! /bin/sh" > $macroname.sh
            echo "export PATH=/cvmfs/sft.cern.ch/lcg/contrib/CMake/3.14.2/Linux-x86_64/bin:\$PATH" >> $macroname.sh
            echo "source /cvmfs/sft.cern.ch/lcg/contrib/gcc/8/x86_64-centos7/setup.sh" >> $macroname.sh
            echo "source /cvmfs/sft.cern.ch/lcg/releases/LCG_96b/ROOT/6.18.04/x86_64-centos7-gcc8-opt/ROOT-env.sh" >> $macroname.sh
            echo "source /cvmfs/geant4.cern.ch/geant4/10.5.p01/x86_64-centos7-gcc8-opt-MT/CMake-setup.sh" >> $macroname.sh
            echo "export HEPMC_DIR=/cvmfs/sft.cern.ch/lcg/releases/LCG_96b/hepmc3/3.1.2/x86_64-centos7-gcc8-opt" >> $macroname.sh
            echo "export FASTJET_DIR=/cvmfs/sft.cern.ch/lcg/releases/LCG_96b/fastjet/3.3.2/x86_64-centos7-gcc8-opt" >> $macroname.sh
            echo "export PYTHIA_DIR=/cvmfs/sft.cern.ch/lcg/releases/LCG_96b/MCGenerators/pythia8/240/x86_64-centos7-gcc8-opt" >> $macroname.sh
            echo "export PYTHIA8=/cvmfs/sft.cern.ch/lcg/releases/LCG_96b/MCGenerators/pythia8/240/x86_64-centos7-gcc8-opt" >> $macroname.sh
            echo "export PYTHIA8DATA=/cvmfs/sft.cern.ch/lcg/releases/LCG_96b/MCGenerators/pythia8/240/x86_64-centos7-gcc8-opt/share/Pythia8/xmldoc" >> $macroname.sh
            echo "export ROOT_INCLUDE_PATH=/cvmfs/sft.cern.ch/lcg/releases/LCG_96b/hepmc3/3.1.2/x86_64-centos7-gcc8-opt/include:\$ROOT_INCLUDE_PATH" >> $macroname.sh
            echo "export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:\$HEPMC_DIR/lib64:\$FASTJET_DIR/lib:\$PYTHIA_DIR/lib:\$PWD/lib" >> $macroname.sh
            echo "./bin/DRsim $macroname.mac \$1 $results/root/$root_name" >> $macroname.sh

            echo "universe = vanilla" > $macroname.sub
            echo "executable = $macroname.sh" >> $macroname.sub
            echo "arguments = \$(ProcId)" >> $macroname.sub
            echo "core_size = 0" >> $macroname.sub
            echo "output = $results/log/\$(ProcId).out" >> $macroname.sub
            echo "error = $results/log/\$(ProcId).err" >> $macroname.sub
            echo "log = $results/log/\$(ProcId).log" >> $macroname.sub
            echo "request_memory = 1.0 GB" >> $macroname.sub
            echo "should_transfer_files = YES" >> $macroname.sub
            echo "when_to_transfer_output = ON_EXIT" >> $macroname.sub
            echo "transfer_input_files =./bin, ./lib, ./init.mac, ./$macroname.mac" >> $macroname.sub
            echo "queue 500" >> $macroname.sub

            mkdir "$results"
            mkdir "$results/log"
            mkdir "$results/root"
            condor_submit $macroname.sub

            echo "Simulation Uploaded"
            echo "$i $j GeV"

            cp $macroname.* $results
        done
    done
done

