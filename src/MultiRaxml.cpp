

#include "MultiRaxml.hpp"
#include "Raxml.hpp"

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <iostream>       
#include <thread>         
#include <chrono>     
#include <ctime>     
#include "log.hpp"
#include "ParallelContext.hpp"

using namespace std;
using Timer =  chrono::time_point<chrono::system_clock>; 
                             
const int MPI_TAG_GET_CMD = 1;

const int MPI_SIGNAL_KILL_MASTER = 1;
const int MPI_SIGNAL_GET_CMD = 2;
const int MPI_SIGNAL_GET_SORTED_CMD = 3;
const int MPI_SIGNAL_SEND_CMD_DIM = 4;

const int NO_MORE_CMDS = -1;

int MASTER_RANK = 0;

void print_elapsed(const Timer &begin)
{
  Timer end = chrono::system_clock::now();
  int elapsed_sec = chrono::duration_cast<chrono::seconds>
    (end-begin).count();
  cout << "Elapsed time : " << elapsed_sec << "s" << endl;
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

class RaxmlCommand {
public:
  RaxmlCommand() :
    _threadsNumber(0),
    _args(0)
  {

  }

  RaxmlCommand(const string &command_str)
  {
    _valid = true;
    istringstream is(command_str);
    _args.push_back("raxml");
    while (is) {
      string str;
      is >> str;
      _args.push_back(str);
    }
    _argv = vector<char *>(_args.size() + 1, 0);
    for (unsigned int i = 0; i < _args.size(); ++i) {
      _argv [i + 1] = (char *)_args[i].c_str(); //todobenoit const hack here
    }
  }

  bool parse_dimensions(int &sites, int &tips) {
    _valid = (EXIT_SUCCESS == 
        get_dimensions(_argv.size() - 1, &_argv[0], sites, tips));
    return _valid;
  }

  void set_dimensions(int sites, int tips) {
    
    _sites = sites;
    _tips = tips;
    _threadsNumber = sitesToThreads(_sites);

  }

  bool valid() const {return _valid;}

  void run(MPI_Comm comm, MPI_Comm globalComm) const {
    ParallelContext::set_comm(comm);
    if (!getRank(comm)) {
      cout << getDebugStr(getRank(globalComm), getSize(comm));
    }
    raxml(_argv.size() - 1, (char **)&_argv[0]); // todobenoit const hack here
  }
  
  string getDebugStr(int globalRank, int threads) const {
    ostringstream os;
    if (!_valid) {
      os << "Invalid command: ";
    } else {
      os << "Proc " << globalRank << ":" << globalRank + threads - 1 << " runs: ";
    }
    for (const auto &str: _args) {
      os << str << " ";
    }
    os << endl;
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
  vector<char *> _argv;
  int _sites;
  int _tips;
  bool _valid;
};
using RaxmlCommands = vector<shared_ptr<RaxmlCommand> >;


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

// master thread
void *master_thread(void *d) {
  MPI_Comm globalComm = (MPI_Comm) d;
  int currentCommand = 0;
  int currentSortedCommand = 0;
  std::vector<DimBuff> dimensions;
  while(true) {
    MPI_Status stat;
    const unsigned int SIZE_BUFF = 4;
    int tmp[SIZE_BUFF];
    int buff_size = 1;
    MPI_Probe(MPI_ANY_SOURCE, MPI_TAG_GET_CMD, globalComm, &stat);
    MPI_Get_count(&stat, MPI_INT, &buff_size);
    MPI_Recv(tmp, buff_size, MPI_INT, MPI_ANY_SOURCE, MPI_TAG_GET_CMD, globalComm, &stat);
    if (tmp[0] == MPI_SIGNAL_KILL_MASTER) {
      cout << "thread_master received kill signal" << endl;
      return 0;
    } else if (tmp[0] == MPI_SIGNAL_SEND_CMD_DIM) {
      DimBuff b;
      b.command = tmp[1];
      b.sites = tmp[2];
      b.nodes = tmp[3];
      dimensions.push_back(b);    
      sort(dimensions.rbegin(), dimensions.rend(), DimBuff::compare); 
    } else if (tmp[0] == MPI_SIGNAL_GET_CMD) {
      MPI_Send(&currentCommand, 1, MPI_INT, stat.MPI_SOURCE, MPI_TAG_GET_CMD, globalComm);
      currentCommand++;
    } else if (tmp[0] == MPI_SIGNAL_GET_SORTED_CMD) {
      if (currentSortedCommand < dimensions.size()) {
        MPI_Send(&dimensions[currentSortedCommand].command,
            3, MPI_INT, stat.MPI_SOURCE, MPI_TAG_GET_CMD, globalComm);
        currentSortedCommand++;
      } else {
        int buff[3];
        buff[0] = buff[1] = buff[2] = NO_MORE_CMDS;
        MPI_Send(buff,
            3, MPI_INT, stat.MPI_SOURCE, MPI_TAG_GET_CMD, globalComm);
      }
    } else {
      cerr << "Error in master_thread: unexpected signal " << tmp << endl; 
    }
  }
  return 0;
}


// called from the slave thread
int getSortedCurrentCommand(RaxmlCommands &commands, MPI_Comm globalComm, MPI_Comm localComm) {
  int tmp[3];
  int requestingRank = 0;
  if (getRank(localComm) == requestingRank) {
    MPI_Status stat;
    MPI_Sendrecv(&MPI_SIGNAL_GET_SORTED_CMD, 1,
        MPI_INT, MASTER_RANK, MPI_TAG_GET_CMD, 
        &tmp, 3, MPI_INT, MASTER_RANK, 
        MPI_TAG_GET_CMD, globalComm, &stat);
  }

  MPI_Bcast(&tmp, 3, MPI_INT, requestingRank, localComm);
  MPI_Barrier(localComm);
  if (tmp[0] == NO_MORE_CMDS) {
    return NO_MORE_CMDS;
  }
  std::shared_ptr<RaxmlCommand> cmd = commands[tmp[0]];
  cmd->set_dimensions(tmp[1], tmp[2]);
  return tmp[0];
}

int getCurrentCommand(MPI_Comm globalComm) {
  int currentCommand = 0;
    MPI_Status stat;
    MPI_Sendrecv(&MPI_SIGNAL_GET_CMD, 1,
        MPI_INT, MASTER_RANK, MPI_TAG_GET_CMD, 
        &currentCommand, 1, MPI_INT, MASTER_RANK, 
        MPI_TAG_GET_CMD, globalComm, &stat);
    return currentCommand;
}

void send_dimensions_to_master(int cmd, int sites, int nodes, MPI_Comm globalComm) {
  int msg[4];
  msg[0] = MPI_SIGNAL_SEND_CMD_DIM;
  msg[1] = cmd;
  msg[2] = sites;
  msg[3] = nodes;
  MPI_Send(msg, 4, MPI_INT, MASTER_RANK, MPI_TAG_GET_CMD, globalComm); 
}

void readCommands(const string &input_file, RaxmlCommands &commands, MPI_Comm globalComm, MPI_Comm localComm)
{
  vector<string> commands_str;
  ifstream is(input_file);
  for(string line; getline( is, line ); ) {
    if (line.size() && line.at(0) != '#') {
      commands_str.push_back(line);
    }
  }
  for (const auto &str: commands_str) {
    RaxmlCommand *command = new RaxmlCommand(str);
    commands.push_back(shared_ptr<RaxmlCommand>(command));
  }
  int currCmd = 0;
  while ((currCmd = getCurrentCommand(globalComm)) < (int)commands.size()) {
    int sites = 0;
    int nodes = 0;
    bool valid = commands[currCmd]->parse_dimensions(sites, nodes);
    if (valid)
      send_dimensions_to_master(currCmd, sites, nodes, globalComm);
  }
}

void slaves_thread(const string &input_file, MPI_Comm globalComm, MPI_Comm localComm, Timer &begin) {
  RaxmlCommands commands;
  int currCommand = 0;
  readCommands(input_file, commands, globalComm, localComm);
  std::cout << "read " << getRank(globalComm) << std::endl;
  MPI_Barrier(localComm);
  if (!getRank(localComm)) {
    print_elapsed(begin);
  }
  while ((currCommand = getSortedCurrentCommand(commands, globalComm, localComm)) != NO_MORE_CMDS) {
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
    cout << "Invalid syntax: multi-raxml requires one argument" << endl;
    return 0;
  }
  Timer begin = chrono::system_clock::now();;
  MPI_Comm globalWorld = MPI_COMM_WORLD;
  int globalRank = getRank(globalWorld);
  MASTER_RANK = getSize(globalWorld) - 1;
  MPI_Comm localWorld;
  MPI_Comm_split(MPI_COMM_WORLD, globalRank == MASTER_RANK, globalRank, &localWorld);
  
  if (MASTER_RANK == globalRank) {
    time_t beginTime = chrono::system_clock::to_time_t(begin);
    cout << "Starting time: " << ctime(&beginTime) << endl;
    master_thread(globalWorld);
  } else {
    string input_file = argv[1]; 
    slaves_thread(input_file, globalWorld, localWorld, begin);
    MPI_Barrier(localWorld);
  }
  if (0 == getRank(localWorld)) {
    MPI_Send(&MPI_SIGNAL_KILL_MASTER, 1, MPI_INT, MASTER_RANK, MPI_TAG_GET_CMD, globalWorld); 
  }
  if (MASTER_RANK == globalRank) {
    print_elapsed(begin);
  }
  return 0; 
}











