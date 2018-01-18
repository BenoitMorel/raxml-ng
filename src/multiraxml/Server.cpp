#include "Server.hpp"
#include "common.hpp"

#include <mpi.h>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

using namespace std;

namespace multiraxml {

struct DimBuff {
  int command;
  int sites;
  int nodes;
  static bool compare(const DimBuff &a, const DimBuff &b) {
    if (a.sites == b.sites) {
      return a.nodes < b.nodes;
    }
    return (a.sites < b.sites); 
  }
};

void server_thread(MPI_Comm globalComm) {
  std::cout << "starting server thread" << std::endl;
  int currentCommand = 0;
  int currentSortedCommand = 0;
  std::vector<DimBuff> dimensions;
  while(true) {
    MPI_Status stat;
    const unsigned int SIZE_BUFF = 4;
    int tmp[SIZE_BUFF];
    int buff_size = 1;
    MPI_Probe(MPI_ANY_SOURCE, MPI_TAG_TO_MASTER, globalComm, &stat);
    MPI_Get_count(&stat, MPI_INT, &buff_size);
    MPI_Recv(tmp, buff_size, MPI_INT, MPI_ANY_SOURCE, MPI_TAG_TO_MASTER, globalComm, &stat);
    if (tmp[0] == MPI_SIGNAL_KILL_MASTER) {
      cout << "thread_master received kill signal" << endl;
      return ;
    } else if (tmp[0] == MPI_SIGNAL_END_DRYRUNS) {
      cout << "sorting commands..." << endl;
      sort(dimensions.rbegin(), dimensions.rend(), DimBuff::compare); 
    } else if (tmp[0] == MPI_SIGNAL_SEND_CMD_DIM) {
      DimBuff b;
      b.command = tmp[1];
      b.sites = tmp[2];
      b.nodes = tmp[3];
      dimensions.push_back(b);    
    } else if (tmp[0] == MPI_SIGNAL_GET_CMD) {
      MPI_Send(&currentCommand, 1, MPI_INT, stat.MPI_SOURCE, MPI_TAG_TO_MASTER, globalComm);
      currentCommand++;
    } else if (tmp[0] == MPI_SIGNAL_GET_SORTED_CMD) {
      if (currentSortedCommand < (int)dimensions.size()) {
        MPI_Send(&dimensions[currentSortedCommand].command,
            3, MPI_INT, stat.MPI_SOURCE, MPI_TAG_TO_MASTER, globalComm);
        currentSortedCommand++;
      } else {
        int buff[3];
        buff[0] = buff[1] = buff[2] = NO_MORE_CMDS;
        MPI_Send(buff,
            3, MPI_INT, stat.MPI_SOURCE, MPI_TAG_TO_MASTER, globalComm);
      }
    } else {
      cerr << "Error in master_thread: unexpected signal " << tmp << endl; 
    }
  }
}


}
