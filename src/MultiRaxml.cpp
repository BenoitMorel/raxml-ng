

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


int MPI_TAG_GET_CMD = 0;
int MPI_SIGNAL_KILL_MASTER = 1;
int MPI_SIGNAL_GET_CMD = 2;

int getRank(MPI_Comm comm)
{
  int res;
  MPI_Comm_rank(comm, &res);
  return res;
}

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
    _args.push_back("raxml");
    while (is) {
      string str;
      is >> str;
      _args.push_back(str);
    }
  }

  void run(MPI_Comm comm) const {
    ParallelContext::set_comm(comm);
    std::vector<char *> argv(_args.size() + 1, 0);
    for (unsigned int i = 0; i < _args.size(); ++i) {
      argv [i + 1] = (char *)_args[i].c_str(); //todobenoit const hack here
    }
    if (!getRank(comm)) {
      std::cout << "Running ";
      printCommand();
    }
    raxml(argv.size() - 1, &argv[0]);
  }
  
  void printCommand() const {
    for (const auto &str: _args) {
      std::cout << str << " ";
    } 
    std::cout << std::endl;
  }

  static bool comparePtr(shared_ptr<RaxmlCommand> a, shared_ptr<RaxmlCommand> b) { 
    return (a->_threadsNumber < b->_threadsNumber); 
  }
    
private:
  int _threadsNumber;
  vector<string> _args;
};
using RaxmlCommands = vector<shared_ptr<RaxmlCommand> >;


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
  while(true) {
    MPI_Status stat;
    int tmp = 0;
    MPI_Recv(&tmp, 1, MPI_INT, MPI_ANY_SOURCE, MPI_TAG_GET_CMD, globalComm, &stat);
    if (tmp == MPI_SIGNAL_KILL_MASTER) {
      std::cout << "thread_master received kill signal" << std::endl;
      return 0;
    } else if (tmp == MPI_SIGNAL_GET_CMD) {
      std::cout << "thread master sends command " << currentCommand << " to rank " << stat.MPI_SOURCE << std::endl;
      MPI_Send(&currentCommand, 1, MPI_INT, stat.MPI_SOURCE, MPI_TAG_GET_CMD, globalComm);
      currentCommand++;
    } else {
      std::cerr << "Error in master_thread: unexpected signal " << tmp << std::endl; 
    }
  }
  return 0;
}


// called from the slave thread
int getCurrentCommand(MPI_Comm globalComm, MPI_Comm localComm) {
  int currentCommand = 0;
  int requestingRank = 0;
  if (getRank(localComm) == requestingRank) {
    MPI_Status stat;
    MPI_Sendrecv(&MPI_SIGNAL_GET_CMD, 1, MPI_INT, 0, MPI_TAG_GET_CMD, 
        &currentCommand, 1, MPI_INT, 0, MPI_TAG_GET_CMD, globalComm, &stat);
  }
  MPI_Bcast(&currentCommand, 1, MPI_INT, requestingRank, localComm);
  return currentCommand;
}

void readCommands(const std::string &input_file, RaxmlCommands &commands)
{
  vector<string> commands_str;
  read_commands_file(input_file, commands_str);
  for (const auto &str: commands_str) {
    commands.push_back(shared_ptr<RaxmlCommand>(new RaxmlCommand(str)));
  }
  std::sort(commands.rbegin(), commands.rend(), RaxmlCommand::comparePtr); 
}

void slaves_thread(const std::string &input_file, MPI_Comm globalComm, MPI_Comm localComm) {
  RaxmlCommands commands;
  int command = 0;
  readCommands(input_file, commands);
  while ((command = getCurrentCommand(globalComm, localComm)) < (int)commands.size()) {
    std::cerr << "Proc " << getRank(globalComm) << " " << " command " << command << std::endl;
    commands[command]->run(localComm);
  }
}

int multi_raxml(int argc, char** argv)
{
  if (argc != 2) {
    std::cout << "Invalid syntax: multi-raxml requires one argument" << endl;
    return 0;
  }
  int rank = getRank(MPI_COMM_WORLD);
  MPI_Comm dupworldGlobal;
  MPI_Comm_dup(MPI_COMM_WORLD, &dupworldGlobal);
  MPI_Comm dupworldLocal;
  MPI_Comm_split(MPI_COMM_WORLD, rank == 0, rank - 1, &dupworldLocal);

  if (!rank) {
    master_thread(dupworldGlobal);
  } else {
    string input_file = argv[1]; 
    slaves_thread(input_file, dupworldGlobal, dupworldLocal);
  }  
  if (0 == getRank(dupworldLocal)) {
    MPI_Send(&MPI_SIGNAL_KILL_MASTER, 1, MPI_INT, 0, MPI_TAG_GET_CMD, dupworldGlobal); 
  }
  return 0; 
}











