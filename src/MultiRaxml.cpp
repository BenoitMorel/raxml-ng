

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

void *master_thread(void *) {
  while(true) {
    MPI_Status stat;
    MPI_Request request;
    int tmp;
    int flag = false;
    MPI_Irecv(&tmp, 1, MPI_INT, MPI_ANY_SOURCE, 42, MPI_COMM_WORLD, &request);
    MPI_Test(&request, &flag, &stat); 
    while(!flag) {
      std::this_thread::sleep_for (std::chrono::milliseconds(20));
      MPI_Test(&request, &flag, &stat); 
    }
    if (tmp == -1) {
      return 0;
    }
    std::cerr << "m send " << currentCommand << std::endl; 
    MPI_Send(&currentCommand, 1, MPI_INT, stat.MPI_SOURCE, 42, MPI_COMM_WORLD);
    std::cerr << "m sent " << currentCommand << std::endl; 
    currentCommand++;
    std::this_thread::sleep_for (std::chrono::milliseconds(20));
  }
  return 0;
}

int getRank(MPI_Comm comm)
{
  int res;
  MPI_Comm_rank(comm, &res);
  return res;
}

int requestCurrentCommand() {
  if (getRank(MPI_COMM_WORLD)) {
    int res = -1;
    int tmp = 0;
    MPI_Status stat;
    std::cerr << "s ask " << std::endl; 
    MPI_Sendrecv(&tmp, 1, MPI_INT, 0, 42, &res, 1, MPI_INT, 0, 42, MPI_COMM_WORLD, &stat);
    std::cerr << "s got " << res << std::endl; 
    return res;
  } else {
    return currentCommand++;
  }
}

int getCurrentCommand(MPI_Comm comm) {
  unsigned int currentCommand = 0;
  if (!getRank(comm)) {
    currentCommand = requestCurrentCommand();
  }
  std::cerr << "bef bcast" << std::endl;
  MPI_Bcast(&currentCommand, 1, MPI_INT, 0, comm);
  std::cerr << "aft bcast" << std::endl;
  return currentCommand;
}

void slaves_thread(const vector<RaxmlCommand *> &commands, MPI_Comm comm) {
  int command = 0;
  while ((command = getCurrentCommand(comm)) < commands.size()) {
    std::cerr << "Proc " << getRank(MPI_COMM_WORLD) << " " << " command " << command << std::endl;
    std::this_thread::sleep_for (std::chrono::milliseconds(1000));
    MPI_Barrier(comm);
    //std::cerr << "after barrier" << std::endl;
  }
  /*
  for (auto command: commands) {
    command->run(MPI_COMM_WORLD);
  }
  */
}

int multi_raxml(int argc, char** argv)
{
  if (argc != 2) {
    LOG_ERROR << "Invalid syntax: multi-raxml requires one argument" << endl;
    return 0;
  }
  
  LOG_INFO << "Running multi_raxml with " << 
    ParallelContext::num_ranks() << "ranks" << std::endl;

  if (!ParallelContext::proc_id()) {
    pthread_t master;
    pthread_create(&master, 0, master_thread, 0);
  }
  string input_file = argv[1]; 
  vector<string> commands_str;
  read_commands_file(input_file, commands_str);
  vector<RaxmlCommand *> commands;
  for (const auto &str: commands_str) {
    commands.push_back(new RaxmlCommand(str));
  }
  std::sort(commands.rbegin(), commands.rend(), RaxmlCommand::comparePtr); 
  
  slaves_thread(commands, MPI_COMM_WORLD);
  
  for (auto command: commands) {
    delete command;
  }
  
  LOG_INFO << "todobenoit: remove this horrible termination hack" << std::endl;
  std::this_thread::sleep_for (std::chrono::milliseconds(1000));
  if (1 == ParallelContext::proc_id()) {
    int stop = -1;
    MPI_Send(&stop, 1, MPI_INT, 0, 42, MPI_COMM_WORLD); 
  }
  std::this_thread::sleep_for (std::chrono::milliseconds(1000));
  return 0; 
}











