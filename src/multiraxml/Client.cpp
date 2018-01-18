

#include "Client.hpp"
#include "common.hpp"

#include <mpi.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

#include "../ParallelContext.hpp"
#include "../Raxml.hpp"

using namespace std;
                             

namespace multiraxml {


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


int getSortedCurrentCommand(RaxmlCommands &commands, MPI_Comm globalComm, MPI_Comm localComm) {
  int tmp[3];
  int requestingRank = 0;
  if (getRank(localComm) == requestingRank) {
    MPI_Status stat;
    MPI_Sendrecv(&MPI_SIGNAL_GET_SORTED_CMD, 1,
        MPI_INT, getMasterRank(globalComm), MPI_TAG_GET_CMD, 
        &tmp, 3, MPI_INT, getMasterRank(globalComm), 
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
        MPI_INT, getMasterRank(globalComm), MPI_TAG_GET_CMD, 
        &currentCommand, 1, MPI_INT, getMasterRank(globalComm), 
        MPI_TAG_GET_CMD, globalComm, &stat);
    return currentCommand;
}

void send_dimensions_to_master(int cmd, int sites, int nodes, MPI_Comm globalComm) {
  int msg[4];
  msg[0] = MPI_SIGNAL_SEND_CMD_DIM;
  msg[1] = cmd;
  msg[2] = sites;
  msg[3] = nodes;
  MPI_Send(msg, 4, MPI_INT, getMasterRank(globalComm), MPI_TAG_GET_CMD, globalComm); 
}

void readCommands(const string &input_file, RaxmlCommands &commands, MPI_Comm globalComm)
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

void client_thread(const string &input_file, MPI_Comm globalComm, MPI_Comm localComm, Timer &begin) {
  
  RaxmlCommands commands;
  int currCommand = 0;
  MPI_Comm initialLocalComm;
  MPI_Comm_dup(localComm, &initialLocalComm);

  readCommands(input_file, commands, globalComm);
  MPI_Barrier(localComm);
  if (!getRank(localComm)) {
    std::cout << "Raxml dry runs finished. ";
    print_elapsed(begin);
  }
  MPI_Barrier(localComm);
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
  
  MPI_Barrier(initialLocalComm);
  if (0 == getRank(localComm)) {
    MPI_Send(&MPI_SIGNAL_KILL_MASTER, 1, MPI_INT, getMasterRank(globalComm), MPI_TAG_GET_CMD, globalComm); 
  }
}



} // namespace









