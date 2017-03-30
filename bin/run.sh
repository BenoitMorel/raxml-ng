#./raxml-ng-mpi --prefix normalrun/normal --seed 42 --simd avx --msa data/59/59.phy --model data/59/59.part --tree data/59/unrooted.newick --threads 4 --redo
./raxml-ng-mpi --prefix cudarun/cuda --seed 42 --simd avx --msa data/59/59.phy --model data/59/59.part --tree data/59/unrooted.newick --threads 1 --cuda --redo

