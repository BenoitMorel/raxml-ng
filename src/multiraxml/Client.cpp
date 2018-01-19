

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


RaxmlCommand::RaxmlCommand() :
  _threadsNumber(0),
  _args(0)
{
}

RaxmlCommand::RaxmlCommand(const string &command_str)
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

bool RaxmlCommand::parse_dimensions(int &sites, int &tips) {
  _valid = (EXIT_SUCCESS == 
      get_dimensions(_argv.size() - 1, &_argv[0], sites, tips));
  //cout << "dims " << sites << endl;
  return _valid;
}

void RaxmlCommand::set_dimensions(int sites, int tips) {
  
  _sites = sites;
  _tips = tips;
  _threadsNumber = sitesToThreads(_sites);

}

void RaxmlCommand::run(MPI_Comm comm, MPI_Comm globalComm) const {
  ParallelContext::set_comm(comm);
  if (!getRank(comm)) {
    cout << getDebugStr(getRank(globalComm), getSize(comm));
  }
  raxml(_argv.size() - 1, (char **)&_argv[0]); // todobenoit const hack here
}

string RaxmlCommand::getDebugStr(int globalRank, int threads) const {
  ostringstream os;
  if (!_valid) {
    os << "Invalid command: ";
  } else {
    os << "Proc " << globalRank << ":" << globalRank + threads << " runs: ";
  }
  for (const auto &str: _args) {
    os << str << " ";
  }
  os << endl;
  return os.str();
}

bool RaxmlCommand::comparePtr(shared_ptr<RaxmlCommand> a, shared_ptr<RaxmlCommand> b) {
if (a->_threadsNumber == b->_threadsNumber) {
    return a->_tips < b->_tips;
  }
  return (a->_threadsNumber < b->_threadsNumber); 
}



Client::Client(MPI_Comm globalComm, MPI_Comm localComm):
  _globalComm(globalComm),
  _initialLocalComm(localComm),
  _globalMasterRank(getSize(globalComm) - 1)
{
  MPI_Comm_dup(_initialLocalComm, &_currentLocalComm);
}

int Client::getSortedCurrentCommand(RaxmlCommands &commands) {
  int tmp[3];
  int requestingRank = 0;
  if (getRank(_currentLocalComm) == requestingRank) {
    MPI_Status stat;
    MPI_Sendrecv(&MPI_SIGNAL_GET_SORTED_CMD, 1,
        MPI_INT, _globalMasterRank, MPI_TAG_TO_MASTER, 
        &tmp, 3, MPI_INT, _globalMasterRank, 
        MPI_TAG_TO_MASTER, _globalComm, &stat);
  }

  MPI_Bcast(&tmp, 3, MPI_INT, requestingRank, _currentLocalComm);
  MPI_Barrier(_currentLocalComm);
  if (tmp[0] == NO_MORE_CMDS) {
    return NO_MORE_CMDS;
  }
  auto cmd = commands[tmp[0]];
  cmd->set_dimensions(tmp[1], tmp[2]);
  return tmp[0];
}

int Client::getCurrentCommand() {
  int currentCommand = 0;
  MPI_Status stat;
  MPI_Sendrecv(&MPI_SIGNAL_GET_CMD, 1,
      MPI_INT, _globalMasterRank, MPI_TAG_TO_MASTER, 
      &currentCommand, 1, MPI_INT, _globalMasterRank, 
      MPI_TAG_TO_MASTER, _globalComm, &stat);
  return currentCommand;
}

void Client::send_dimensions_to_master(int cmd, int sites, int nodes) {
  int msg[4];
  msg[0] = MPI_SIGNAL_SEND_CMD_DIM;
  msg[1] = cmd;
  msg[2] = sites;
  msg[3] = nodes;
  MPI_Send(msg, 4, MPI_INT, _globalMasterRank, MPI_TAG_TO_MASTER, _globalComm); 
}

void Client::readCommands(const string &input_file, RaxmlCommands &commands)
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
  while ((currCmd = getCurrentCommand()) < (int)commands.size()) {
    int sites = 0;
    int nodes = 0;
    bool valid = commands[currCmd]->parse_dimensions(sites, nodes);
    if (valid) {
      send_dimensions_to_master(currCmd, sites, nodes);
    }
  }
  MPI_Barrier(_initialLocalComm);
  if (0 == getRank(_initialLocalComm)) {
    MPI_Send(&MPI_SIGNAL_END_DRYRUNS, 1, MPI_INT, _globalMasterRank, MPI_TAG_TO_MASTER, _globalComm); 
  }
}

void Client::sendStats(const Stats &s) 
{
  if (getRank(_currentLocalComm)) {
    return;
  }
  int tmp[5];
  tmp[0] = MPI_SIGNAL_SEND_STATS;
  *((Stats*)&tmp[1]) = s;
  MPI_Send(&tmp, 5, MPI_INT, _globalMasterRank, MPI_TAG_TO_MASTER, _globalComm);  
}

void Client::client_thread(const string &input_file, Timer &begin, bool naive) {
  
  RaxmlCommands commands;
  int currCommand = 0;
  readCommands(input_file, commands);
  MPI_Barrier(_initialLocalComm);
  if (!getRank(_initialLocalComm)) {
    std::cout << "Raxml dry runs finished. ";
    print_elapsed(begin);
  }
  MPI_Barrier(_initialLocalComm);
  if (naive) { // one run per rank: let's split everything
    MPI_Comm newComm;
    MPI_Comm_split(_currentLocalComm, getRank(_currentLocalComm), 0, &newComm);
    _currentLocalComm = newComm;
  }
  while ((currCommand = getSortedCurrentCommand(commands)) != NO_MORE_CMDS) {
    auto command = commands[currCommand];
    bool runTheCommand = true;
    while (command->getThreadsNumber() < getSize(_currentLocalComm)) {
      MPI_Comm newComm;
      int size = getSize(_currentLocalComm);
      int rank = getRank(_currentLocalComm);
      int color = rank < ((size + 1) / 2);
      int key = rank % ((size + 1) / 2);
      MPI_Comm_split(_currentLocalComm, color, key, &newComm);
      runTheCommand &= color;
      _currentLocalComm = newComm;
    }
    if (runTheCommand) {
      Stats s;
      s.startingTime = get_elapsed_ms(begin);
      s.startingRank = getRank(_globalComm);
      s.ranks = getSize(_currentLocalComm);
      Timer commandBegin = chrono::system_clock::now();
      command->run(_currentLocalComm, _globalComm);
      MPI_Barrier(_currentLocalComm);
      s.duration = get_elapsed_ms(commandBegin);
      sendStats(s);
      MPI_Barrier(_currentLocalComm);
    }
  }
  
  MPI_Barrier(_initialLocalComm);
  if (0 == getRank(_initialLocalComm)) {
    MPI_Send(&MPI_SIGNAL_KILL_MASTER, 1, MPI_INT, _globalMasterRank, MPI_TAG_TO_MASTER, _globalComm); 
  }
}



} // namespace









