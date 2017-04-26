#include "Bench.hpp"

void Bench::bench(TreeInfo& treeinfo)
{
  const unsigned int iterations = 1000;
  LOG_INFO << "BEGIN BENCH" << std::endl;

  for (unsigned int i = 0; i < iterations; ++i) {
    if (i % 100) {
      LOG_INFO << i << std::endl;
    }
    treeinfo.recompute_likelihood();
  }
  LOG_INFO << "END BENCH" << std::endl;
}

