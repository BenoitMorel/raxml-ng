

#include "MultiRaxml.hpp"
#include "Raxml.hpp"

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <iostream>       // std::cout, std::endl
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include "log.hpp"
#include "ParallelContext.hpp"

using namespace std;



class RaxmlCommand {
public:
  RaxmlCommand() :
    _threadsNumber(0),
    _args(0)
  {

  }

  RaxmlCommand(const std::string &command_str) :
    _threadsNumber(0),
    _args(0)
  {
    istringstream is(command_str);
    is >> _threadsNumber;
    //LOG_INFO << "Threads : " << _threadsNumber << endl;
    _args.push_back("raxml");
    while (is) {
      string str;
      is >> str;
      _args.push_back(str);
   //   LOG_INFO << str << endl;
    }
  }

  void run(MPI_Comm comm) const {
    ParallelContext::set_comm(comm);
    std::vector<char *> argv(_args.size() + 1, 0);
    for (unsigned int i = 0; i < _args.size(); ++i) {
      argv [i + 1] = (char *)_args[i].c_str(); //todobenoit const hack here
    }

    raxml(argv.size() - 1, &argv[0]);
  }
  
  static bool comparePtr(RaxmlCommand* a, RaxmlCommand* b) { 
    return (a->_threadsNumber < b->_threadsNumber); 
  }
    
private:
  int _threadsNumber;
  vector<string> _args;
};


void read_commands_file(const string &input_file,
    vector<string> &commands)
{
  ifstream is(input_file);
  for(string line; getline( is, line ); ) {
    if (line.size() && line.at(0) != '#') {
      commands.push_back(line);
    }
  }
}

unsigned int currentCommand = 0;

// master thread
void *master_thread(void *d) {
  MPI_Comm globalComm = (MPI_Comm) d;
  //logger().add_log_stream(&std::cout);
  while(true) {
    MPI_Status stat;
    MPI_Request request;
    int tmp;
    MPI_Irecv(&tmp, 1, MPI_INT, MPI_ANY_SOURCE, 42424242, globalComm, &request);
    MPI_Wait(&request, &stat);
    /*
    int flag = false;
    MPI_Test(&request, &flag, &stat); 
    while(!flag) {
      std::this_thread::sleep_for (std::chrono::milliseconds(100));
      MPI_Test(&request, &flag, &stat); 
    }
    */
    if (tmp != 37 && tmp != -1) {
      std::cout << "ERROR IN MASTER THREAD, wrong magic number " << tmp << std::endl;
    }
    if (tmp == -1) {
      std::cout << "KILL MASTER" << std::endl;
      return 0;
    }
    std::cout << "send command " << currentCommand << " to rank " << stat.MPI_SOURCE << std::endl;
    MPI_Send(&currentCommand, 1, MPI_INT, stat.MPI_SOURCE, 42424242, globalComm);
    currentCommand++;
  }
  return 0;
}

int getRank(MPI_Comm comm)
{
  int res;
  MPI_Comm_rank(comm, &res);
  return res;
}

// slave thread
int requestCurrentCommand(MPI_Comm globalComm) {
  int res = -1;
  int tmp = 37;
  MPI_Status stat;
  MPI_Sendrecv(&tmp, 1, MPI_INT, 0, 42424242, &res, 1, MPI_INT, 0, 42424242, globalComm, &stat);
  std::cout << "slave send receieve" << std::endl;
  return res;
}

int getCurrentCommand(MPI_Comm globalComm, MPI_Comm localComm) {
  unsigned int currentCommand = 0;
  if (getRank(localComm) == 1) {
    currentCommand = requestCurrentCommand(globalComm);
  }
  MPI_Bcast(&currentCommand, 1, MPI_INT, 1, localComm);
  MPI_Barrier(localComm); // todobenoit useless
  std::cout << "bcast" << std::endl;
  return currentCommand;
}

void slaves_thread(const std::string &input_file, MPI_Comm globalComm, MPI_Comm localComm) {
  vector<string> commands_str;
  read_commands_file(input_file, commands_str);
  vector<RaxmlCommand *> commands;
  for (const auto &str: commands_str) {
    commands.push_back(new RaxmlCommand(str));
  }
  std::sort(commands.rbegin(), commands.rend(), RaxmlCommand::comparePtr); 
  int command = 0;
  while ((command = getCurrentCommand(globalComm, localComm)) < (int)commands.size()) {
    std::cerr << "Proc " << getRank(globalComm) << " " << " command " << command << std::endl;
    commands[command]->run(localComm);
  }
  for (auto command: commands) {
    delete command;
  }
}

int multi_raxml(int argc, char** argv)
{
  if (argc != 2) {
    LOG_ERROR << "Invalid syntax: multi-raxml requires one argument" << endl;
    return 0;
  }
  int rank = getRank(MPI_COMM_WORLD);
  MPI_Comm dupworldGlobal;
  MPI_Comm_dup(MPI_COMM_WORLD, &dupworldGlobal);
    //std::this_thread::sleep_for (std::chrono::milliseconds(1000));
  MPI_Comm dupworldLocal;
  MPI_Comm_split(MPI_COMM_WORLD, rank == 0, rank - 1, &dupworldLocal);

  if (!rank) {
    master_thread(dupworldGlobal);
    //pthread_t master;
    //pthread_create(&master, 0, master_thread, dupworldGlobal);
  } else {
    string input_file = argv[1]; 
    slaves_thread(input_file, dupworldGlobal, dupworldLocal);
  }  
  LOG_INFO << "todobenoit: remove this horrible termination hack" << std::endl;
  if (0 == getRank(dupworldLocal)) {
    int stop = -1;
    MPI_Send(&stop, 1, MPI_INT, 0, 42424242, dupworldGlobal); 
  }
  std::cout << "Rank " << rank << " finished " << std::endl;
  return 0; 
}











