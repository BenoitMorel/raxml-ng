#!/bin/bash
#SBATCH -o ng_%j.out
#SBATCH -N 32
#SBATCH -n 512
#SBATCH -B 2:8:1
#SBATCH --threads-per-core=1
#SBATCH --cpus-per-task=1
#SBATCH -t 2:00:00
#  blabla SBATCH --qos=debug
# datas: 128  1kyte_hyme  404  59  antl_1_1_aa  antl_1_1_nt2  kyte  para_1_aa   para_1_nt

mpi=1
jem=1
threads=512
data=para_1_nt
datadirprefix=
datadirsuffix=
optindex=1 #0 repeats
           #1 tipinner
           #2 no optimization
resultsfile=scalability_results
seed=43


source /etc/profile.d/modules.sh

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

mkdir -p $resultsfile/$data
datadir=$resultsfile/$data/${datadirprefix}${data}_${optname[$optindex]}_T$threads${mpisuffix}${jemsuffix}$datadirsuffix
rm -r $datadir
mkdir -p $datadir

LD_PRELOAD=$preload $mpirun ./raxml-ng-mpi --search --seed=$seed --msa data/$data/${data}.phy --model data/$data/${data}.part --simd AVX --threads $mpithreads  ${opt[$optindex]}   --prefix ${datadir}/$data

