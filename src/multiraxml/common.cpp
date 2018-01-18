#include "common.hpp"

#include <iostream>
#include <cmath>

using namespace std;

namespace multiraxml {

void print_elapsed(const Timer &begin)
{
  Timer end = chrono::system_clock::now();
  int elapsed_sec = chrono::duration_cast<chrono::seconds>
    (end-begin).count();
  cout << "Elapsed time : " << elapsed_sec << "s" << endl;
}

int get_elapsed_ms(const Timer &begin)
{
  Timer end = chrono::system_clock::now();
  return chrono::duration_cast<chrono::milliseconds>
    (end-begin).count();
}

int getRank(MPI_Comm comm)
{
  int res;
  MPI_Comm_rank(comm, &res);
  return res;
}

int getSize(MPI_Comm comm)
{
  int res;
  MPI_Comm_size(comm, &res);
  return res;
}

int sitesToThreads(int sites)
{
  int threads = max(sites, 1000) / 1000;
  // lowest power of 2
  return pow(2,floor(log(threads)/log(2)));
}

int getMasterRank(MPI_Comm comm)
{
    return getSize(comm) - 1;
}

int isMasterRank(int rank, MPI_Comm comm)
{
  return rank == getMasterRank(comm);
}
 
} // namespace
