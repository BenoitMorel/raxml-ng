#ifndef _MULTIRAXML_CLIENT_HPP
#define _MULTIRAXML_CLIENT_HPP

#include <mpi.h>
#include "common.hpp"
#include <string>
#include <vector>
#include <memory>

namespace multiraxml {

class RaxmlCommand {
public:
  RaxmlCommand();
  RaxmlCommand(const std::string &command_str);
  bool parse_dimensions(int &sites, int &tips);
  void set_dimensions(int sites, int tips);
  bool valid() const {return _valid;}
  void run(MPI_Comm comm, MPI_Comm globalComm) const;
  std::string getDebugStr(int globalRank, int threads) const;
  static bool comparePtr(std::shared_ptr<RaxmlCommand> a, std::shared_ptr<RaxmlCommand> b);
  int getThreadsNumber() const {return _threadsNumber;}
private:
  int _threadsNumber;
  std::vector<std::string> _args;
  std::vector<char *> _argv;
  int _sites;
  int _tips;
  bool _valid;
};
using RaxmlCommands = std::vector<std::shared_ptr<RaxmlCommand> >;

class Client {
public:
  Client(MPI_Comm globalComm, MPI_Comm localComm);
  void client_thread(const std::string &input_file, Timer &begin); 

private:
  int getSortedCurrentCommand(RaxmlCommands &commands);
  int getCurrentCommand();
  void send_dimensions_to_master(int cmd, int sites, int nodes);
  void readCommands(const std::string &input_file, RaxmlCommands &commands);



  MPI_Comm _globalComm;
  MPI_Comm _currentLocalComm;
  MPI_Comm _initialLocalComm;
  int _globalMasterRank;
};

} // namespace


#endif

