#!/bin/bash
#SBATCH -o ng_T16_%j.out
#SBATCH -N 2
#SBATCH -n 32
#SBATCH -B 2:8:1
#SBATCH --threads-per-core=1
#SBATCH --cpus-per-task=1
#SBATCH -t 24:00:00
 
# datas: 128  1kyte_hyme  404  59  antl_1_1_aa  antl_1_1_nt2  kyte  para_1_aa   para_1_nt

mpi=1
jem=1
threads=32
data=antl_1_1_nt2
datadirprefix=
datadirsuffix=
optindex=1 #0 repeats
           #1 tipinner
           #2 no optimization
seed=42






source /etc/profile.d/modules.sh
module load gompi


preload=
jemsuffix=
if [ $jem = "1" ]
then
  preload=/home/morelbt/github/raxml-ng/bin/libjemalloc.so
  jemsuffix=_jem
fi

if [ $mpi = "1" ]
then
  mpisuffix="_mpi"
  mpirun="mpirun"
  mpithreads=1
else
  mpisuffix="_pthread"
  mpirun=
  mpithreads=$threads
fi

opt=("--repeats on" "" "--tip-inner off")
optname=("repeats" "tipinner" "noopt")

datadir=mpiresults/${datadirprefix}${data}_${optname[$optindex]}_T$threads${mpisuffix}${jemsuffix}$datadirsuffix
rm -r $datadir
mkdir -p $datadir

LD_PRELOAD=$preload $mpirun ./raxml-ng-mpi --search --seed=$seed --msa data/$data/${data}.phy --model data/$data/${data}.part --simd AVX --threads $mpithreads  ${opt[$optindex]}   --prefix ${datadir}/$data

