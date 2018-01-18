#ifndef _MULTIRAXML_COMMON_HPP
#define _MULTIRAXML_COMMON_HPP

#include <chrono>     
#include <mpi.h>


namespace multiraxml {

using Timer =  std::chrono::time_point<std::chrono::system_clock>; 

const int MPI_TAG_TO_MASTER = 1;
const int MPI_SIGNAL_KILL_MASTER = 1;
const int MPI_SIGNAL_GET_CMD = 2;
const int MPI_SIGNAL_GET_SORTED_CMD = 3;
const int MPI_SIGNAL_SEND_CMD_DIM = 4;
const int NO_MORE_CMDS = -1;



void print_elapsed(const Timer &begin);
int getRank(MPI_Comm comm);
int getSize(MPI_Comm comm);
int sitesToThreads(int sites);
int getMasterRank(MPI_Comm comm);
int isMasterRank(int rank, MPI_Comm comm);

} //namespace
#endif
