
#include "multiraxml.hpp"
#include "common.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include "../ParallelContext.hpp"

#include <iostream>
#include <string>
#include <ctime>   
#include <mpi.h>

using namespace std;

namespace multiraxml {

int multiraxml(int argc, char** argv)
{ 
  if (argc != 2) {
    cout << "Invalid syntax: multi-raxml requires one argument" << endl;
    return 0;
  }
    
  string input_file = argv[1]; 
  MPI_Comm savedComm = ParallelContext::get_comm();
  Timer begin = chrono::system_clock::now();;
  
  
  // create a local communicator for the clients
  // the global communicator includes clients and the server
  MPI_Comm globalComm = MPI_COMM_WORLD;
  int globalRank = getRank(globalComm);
  MPI_Comm localComm;
  MPI_Comm_split(MPI_COMM_WORLD, isMasterRank(globalRank, globalComm), globalRank, &localComm);
 
  if (isMasterRank(globalRank, globalComm)) {
    time_t timer_begin = chrono::system_clock::to_time_t(begin);
    cout << "Starting time: " << ctime(&timer_begin) << endl;
    server_thread(globalComm);
    print_elapsed(begin);
  } else {
    client_thread(input_file, globalComm, localComm, begin);
  }
  
  ParallelContext::set_comm(savedComm);
  return 0; 
}

} // end namespace
