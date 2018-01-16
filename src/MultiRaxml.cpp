

#include "MultiRaxml.hpp"
#include "Raxml.hpp"

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <iostream>       // std::cout, std::endl
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include "log.hpp"
#include "ParallelContext.hpp"

using namespace std;


const int MPI_TAG_GET_CMD = 0;
const int MPI_SIGNAL_KILL_MASTER = 1;
const int MPI_SIGNAL_GET_CMD = 2;
int MASTER_RANK = 0;

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
  int threads = std::max(sites, 1000) / 1000;
  // lowest power of 2
  return pow(2,floor(log(threads)/log(2)));
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
    _args.push_back("raxml");
    while (is) {
      string str;
      is >> str;
      _args.push_back(str);
    }
    _argv = std::vector<char *>(_args.size() + 1, 0);
    for (unsigned int i = 0; i < _args.size(); ++i) {
      _argv [i + 1] = (char *)_args[i].c_str(); //todobenoit const hack here
    }

    _valid = (EXIT_SUCCESS == 
        get_dimensions(_argv.size() - 1, &_argv[0], _sites, _tips));
    _threadsNumber = sitesToThreads(_sites);
    //std::cout << _sites << " sites -> " << _threadsNumber << " threads" << std::endl;
  }

  void run(MPI_Comm comm, MPI_Comm globalComm) const {
    if (!_valid) {
      cerr << "Warning: invalid raxml command. Skipping." << endl;
      return;
    }
    ParallelContext::set_comm(comm);
    if (!getRank(comm)) {
      cout << getDebugStr(getRank(globalComm), getSize(comm));
    }
    raxml(_argv.size() - 1, (char **)&_argv[0]); // todobenoit const hack here
  }
  
  std::string getDebugStr(int globalRank, int threads) const {
    ostringstream os;
    os << "Proc " << globalRank << ":" << globalRank + threads - 1 << " runs: ";
    for (const auto &str: _args) {
      os << str << " ";
    }
    os << std::endl;
    return os.str();
  }

  static bool comparePtr(shared_ptr<RaxmlCommand> a, shared_ptr<RaxmlCommand> b) {
    if (a->_threadsNumber == b->_threadsNumber) {
      return a->_tips < b->_tips;
    }
    return (a->_threadsNumber < b->_threadsNumber); 
  }

  int getThreadsNumber() const {return _threadsNumber;}
    
private:
  int _threadsNumber;
  vector<string> _args;
  std::vector<char *> _argv;
  int _sites;
  int _tips;
  bool _valid;
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


// master thread
void *master_thread(void *d) {
  MPI_Comm globalComm = (MPI_Comm) d;
  int currentCommand = 0;
  while(true) {
    MPI_Status stat;
    int tmp = 0;
    MPI_Recv(&tmp, 1, MPI_INT, MPI_ANY_SOURCE, MPI_TAG_GET_CMD, globalComm, &stat);
    if (tmp == MPI_SIGNAL_KILL_MASTER) {
      std::cout << "thread_master received kill signal" << std::endl;
      return 0;
    } else if (tmp == MPI_SIGNAL_GET_CMD) {
      std::cout << "thread master sends command " << currentCommand <<
        " to rank " << stat.MPI_SOURCE << std::endl;
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
    MPI_Sendrecv(&MPI_SIGNAL_GET_CMD, 1, MPI_INT, MASTER_RANK, MPI_TAG_GET_CMD, 
        &currentCommand, 1, MPI_INT, MASTER_RANK, MPI_TAG_GET_CMD, globalComm, &stat);
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
  int currCommand = 0;
  readCommands(input_file, commands);
  while ((currCommand = getCurrentCommand(globalComm, localComm)) < (int)commands.size()) {
    auto command = commands[currCommand];
    bool runTheCommand = true;
    while (command->getThreadsNumber() < getSize(localComm)) {
      MPI_Comm newComm;
      int size = getSize(localComm);
      int rank = getRank(localComm);
      int color = rank < (size / 2);
      int key = rank % (size / 2);
      MPI_Comm_split(localComm, color, key, &newComm);
      runTheCommand &= !color;
      localComm = newComm;
    }
    if (runTheCommand) {
      command->run(localComm, globalComm);
    }
  }
}

int multi_raxml(int argc, char** argv)
{
  if (argc != 2) {
    std::cout << "Invalid syntax: multi-raxml requires one argument" << endl;
    return 0;
  }
  MPI_Comm globalWorld = MPI_COMM_WORLD;
  int globalRank = getRank(globalWorld);
  MASTER_RANK = getSize(globalWorld) - 1;
  MPI_Comm localWorld;
  MPI_Comm_split(MPI_COMM_WORLD, globalRank == MASTER_RANK, globalRank, &localWorld);
  
  if (MASTER_RANK == globalRank) {
    master_thread(globalWorld);
  } else {
    string input_file = argv[1]; 
    slaves_thread(input_file, globalWorld, localWorld);
    MPI_Barrier(localWorld);
  }
  if (0 == getRank(localWorld)) {
    MPI_Send(&MPI_SIGNAL_KILL_MASTER, 1, MPI_INT, MASTER_RANK, MPI_TAG_GET_CMD, globalWorld); 
  }
  std::cout << "rank " << globalRank << " termidated" << std::endl;
  return 0; 
}











