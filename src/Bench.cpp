#include "Bench.hpp"

void Bench::bench(TreeInfo& treeinfo)
{
  ParallelContext::reinit_stats("bench");
  const unsigned int iterations = 5000 / 100;

  LOG_INFO << "BEGIN BENCH" << std::endl;
  double ll = 0.0;
  double total_time_ms = 0;
  double update_time_ms = 0;
  for (unsigned int i = 0; i < iterations; ++i) {
    long t1, t2;
    ll = treeinfo.recompute_likelihood(1, t1, t2);
    total_time_ms  += t1 / 1000000;
    update_time_ms += t2 / 1000000;
  }
  
  double repeats_compression = treeinfo.get_repeats_compression();
  std::vector<double> repeats_compressions(ParallelContext::num_ranks());
  ParallelContext::gather(repeats_compression, &repeats_compressions[0], 0); 
  for (unsigned int i = 0; i < ParallelContext::num_ranks(); ++i) {
    LOG_INFO << "(p" << i << ", " << repeats_compressions[i] << ") ";
  }
  LOG_INFO << std::endl;
  
      
  ParallelContext::parallel_reduce(&total_time_ms, 1, PLLMOD_TREE_REDUCE_SUM);  
  ParallelContext::parallel_reduce(&update_time_ms, 1, PLLMOD_TREE_REDUCE_SUM); 
  ParallelContext::print_stats();
  LOG_INFO << "Total time: " << total_time_ms << "ms" << std::endl;
  LOG_INFO << "Update time: " << update_time_ms<< "ms" << std::endl;
  LOG_INFO << "Master ll = " << ll << std::endl;
  ParallelContext::parallel_reduce(&ll, 1, PLLMOD_TREE_REDUCE_SUM);  
  LOG_INFO << "GLobal ll = " << ll << std::endl;
  LOG_INFO << "END BENCH" << std::endl;
}

