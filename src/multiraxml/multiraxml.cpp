
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
  if (argc != 3) {
    cout << "Invalid syntax: multi-raxml requires 2 arguments" << endl;
    cout << "Syntax: ./multiraxml commandsFile svgoutput" << endl;
    return 0;
  }
    
  string input_file = argv[1]; 
  string svg_output_file = argv[2]; 
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
    Server server(globalComm);
    server.server_thread();
    server.saveStats(svg_output_file);
    print_elapsed(begin);
  } else {
    Client client(globalComm, localComm); 
    client.client_thread(input_file, begin);
  }
  
  ParallelContext::set_comm(savedComm);
  return 0; 
}

} // end namespace
