#!/bin/bash

logy=0 #0:false, 1:set log y
isPbPb=0 #0:pp, 1:PbPb
fillHisto=0 #0: draw histo, 1: fill histo, 2: remove plots, 3,4: 3D eff calculation
useTnP=0 #0: don't apply tnp, 1: old TnP, 2: new TnP with STA * (muID*trig), 3: new TnP with muID*trig only

#directory1=(Rap0.0-1.6_Pt6.5-30.0 Rap0.0-2.4_Pt6.5-30.0 Rap1.6-2.4_Pt6.5-30.0 Rap1.6-2.4_Pt3.0-6.5 Rap1.6-2.4_Pt3.0-30.0)
#directory1=(Rap1.6-2.4_Pt3.0-6.5 Rap1.6-2.4_Pt6.5-30.0 Rap1.6-2.4_Pt3.0-30.0) 
#directory1=(Rap0.0-2.4_Pt6.5-30.0)
#directory2=(notAbs_Rap-2.4--1.6_Pt3.0-30.0)
#directory2=(notAbs_Rap1.6-2.4_Pt3.0-30.0)
#directory2=(notAbs_Rap-1.6-0.0_Pt6.5-30.0)
#directory2=(notAbs_Rap0.0-1.6_Pt6.5-30.0)
#directory2=(notAbs_Rap0.0-1.6_Pt6.5-30.0 notAbs_Rap-1.6-0.0_Pt6.5-30.0)
directory2=(notAbs_Rap0.0-1.6_Pt6.5-30.0 notAbs_Rap-1.6-0.0_Pt6.5-30.0 notAbs_Rap-2.4--1.6_Pt3.0-30.0 notAbs_Rap1.6-2.4_Pt3.0-30.0)
#directory2=(notAbs_Rap-2.4--1.6_Pt3.0-6.5 notAbs_Rap1.6-2.4_Pt3.0-6.5) # notAbs_Rap-2.4--1.6_Pt6.5-30.0 notAbs_Rap1.6-2.4_Pt6.5-30.0)
#directory2=(notAbs_Rap-2.4--1.6_Pt3.0-30.0 notAbs_Rap1.6-2.4_Pt3.0-30.0) # notAbs_Rap-2.4--1.6_Pt6.5-30.0 notAbs_Rap1.6-2.4_Pt6.5-30.0 notAbs_Rap-2.4--1.6_Pt3.0-6.5 notAbs_Rap1.6-2.4_Pt3.0-6.5)
#directory2=(notAbs_Rap0.0-1.2_Pt6.5-30.0 notAbs_Rap-1.2-0.0_Pt6.5-30.0 notAbs_Rap1.2-1.6_Pt6.5-30.0 notAbs_Rap-1.6--1.2_Pt6.5-30.0 )
directory3=(Rap1.6-2.4_Pt6.5-30.0 Rap1.6-2.4_Pt3.0-6.5)
motherdir=$(pwd)

function FillUpHistos {
  ### Arguments
  absRap=$1
  
  ### Do work
  rm lJpsiEff.h lJpsiEff.cpp 3DEff.cpp LxyTrueReco.cpp
#  rm *.log *root
  ln -s ../lJpsiEff.h .
  ln -s ../lJpsiEff.cpp .
  ln -s ../3DEff.cpp .
  ln -s ../LxyTrueReco.cpp .
  pwd
  g++ 3DEff.cpp -o 3DEff `root-config --cflags --glibs`
  ./3DEff $absRap 0 $isPbPb $useTnP >&prmc3D.log&
  ./3DEff $absRap 1 $isPbPb $useTnP >&npmc3D.log&
  g++ lJpsiEff.cpp -o lJpsiEff `root-config --cflags --glibs`
  ./lJpsiEff $absRap 0 $isPbPb $useTnP >&prmc.log&
  ./lJpsiEff $absRap 1 $isPbPb $useTnP >&npmc.log&
}

function DrawHistos {
  ### Arguments
  absRap=$1
  
  ### Do work
  rm -f *pdf *png *~
  rm 3DEff_draw.cpp lJpsiEff_draw.cpp LxyEff_draw.cpp
  ln -s ../3DEff_draw.cpp .
  ln -s ../lJpsiEff_draw.cpp .
  ln -s ../LxyEff_draw.cpp .
  pwd
  g++ 3DEff_draw.cpp -o 3DEff_draw `root-config --cflags --glibs`
  g++ LxyEff_draw.cpp -o LxyEff_draw `root-config --cflags --glibs`
#    g++ lJpsiEff_draw.cpp -o lJpsiEff_draw $cflags $glibs
#    ./lJpsiEff_draw $absRap $logy $isPbPb
  ./3DEff_draw $absRap $logy $isPbPb
  ./LxyEff_draw $absRap $logy $isPbPb
}

if [ $fillHisto -eq 1 ]; then
  for dir in ${directory1[@]}; do
    if [ ! -d $dir ]; then
      mkdir $dir
    fi
    cd $dir
    FillUpHistos 1
    cd $motherdir
  done
    
  for dir in ${directory2[@]}; do
    if [ ! -d $dir ]; then
      mkdir $dir
    fi
    cd $dir
    FillUpHistos 0
    cd $motherdir
  done

elif [ $fillHisto -eq 0 ]; then
  for dir in ${directory1[@]}; do
    cd $dir
    DrawHistos 1
    cd $motherdir
  done

  for dir in ${directory2[@]}; do
    cd $dir
    DrawHistos 0
    cd $motherdir
  done

elif [ $fillHisto -eq 2 ]; then
  for dir in ${directory1[@]}; do
    cd $dir
    rm -f *pdf *png *~ a.out AutoDict_pair_string_string_.cxx
    cd $motherdir
  done
  for dir in ${directory2[@]}; do
    cd $dir
    rm -f *pdf *png *~ a.out AutoDict_pair_string_string_.cxx
    cd $motherdir
  done

elif [ $fillHisto -eq 3 ]; then
  for dir3 in ${directory3[@]}; do
    cd $dir3
    FillUpHistos 1
    cd $motherdir
  done
   
elif [ $fillHisto -eq 4 ]; then
  for dir3 in ${directory3[@]}; do
    cd $dir3
    DrawHistos 1
    cd $motherdir
  done

fi
