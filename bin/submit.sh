#!/bin/bash
#SBATCH -o ng_T16_%j.out
#SBATCH -N 1
#SBATCH -n 1
#SBATCH -B 2:8:1
#SBATCH --threads-per-core=1
#SBATCH --cpus-per-task=16
#SBATCH -t 24:00:00
 
source /etc/profile.d/modules.sh

threads=16
data=para_1_aa
datadirsuffix=_realloc_1
optindex=1 #0 repeats
           #1 tipinner
           #2 no optimization
seed=42

opt=("--repeats on" "" "--tip-inner off")
optname=("repeats" "tipinner" "noopt")

datadir=/home/morelbt/github/raxml-ng/bin/results/${data}_${optname[$optindex]}_T$threads$datadirsuffix
rm -r $datadir
cp -r data/${data} $datadir
./raxml-ng-mpi --seed=$seed --msa $datadir/${data}.phy --model $datadir/${data}.part --simd AVX --threads $threads  ${opt[$optindex]}  --redo 
rm -r ${datadir}_finished
mv $datadir ${datadir}_finished

