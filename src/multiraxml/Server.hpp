#ifndef _MULTIRAXML_SERVER_HPP
#define _MULTIRAXML_SERVER_HPP

#include <mpi.h>
#include <string>
#include <vector>
#include "common.hpp"

namespace multiraxml {


class Server {

public:
  Server(MPI_Comm globalComm);
  void server_thread();
  void saveStats(const std::string &outputFile);
private:
  MPI_Comm _globalComm;
  std::vector<Stats> _stats;
};

} //namespace

#endif
