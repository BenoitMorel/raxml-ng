#include "Server.hpp"
#include "common.hpp"

#include <mpi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstdlib>

using namespace std;

namespace multiraxml {

struct DimBuff {
  int command;
  int sites;
  int nodes;
  static bool compare(const DimBuff &a, const DimBuff &b) {
    int athreads = sitesToThreads(a.sites);
    int bthreads = sitesToThreads(b.sites);
    if (athreads == bthreads) {
      return a.nodes * a.sites > b.nodes * b.sites;
    }
    return (athreads < bthreads); 
  }
};


Server::Server(MPI_Comm globalComm):
  _globalComm(globalComm)
{

}

void Server::server_thread() {
  std::cout << "starting server thread" << std::endl;
  int currentCommand = 0;
  int currentSortedCommand = 0;
  std::vector<DimBuff> dimensions;
  while(true) {
    MPI_Status stat;
    const unsigned int SIZE_BUFF = 5;
    int tmp[SIZE_BUFF];
    int buff_size = 1;
    MPI_Probe(MPI_ANY_SOURCE, MPI_TAG_TO_MASTER, _globalComm, &stat);
    MPI_Get_count(&stat, MPI_INT, &buff_size);
    MPI_Recv(tmp, buff_size, MPI_INT, MPI_ANY_SOURCE, MPI_TAG_TO_MASTER, _globalComm, &stat);
    if (tmp[0] == MPI_SIGNAL_KILL_MASTER) {
      cout << "thread_master received kill signal" << endl;
      return ;
    } else if (tmp[0] == MPI_SIGNAL_END_DRYRUNS) {
      cout << "sorting commands..." << endl;
      sort(dimensions.rbegin(), dimensions.rend(), DimBuff::compare); 
    } else if (tmp[0] == MPI_SIGNAL_SEND_CMD_DIM) {
      DimBuff b;
      b.command = tmp[1];
      b.sites = tmp[2];
      b.nodes = tmp[3];
      dimensions.push_back(b);    
    } else if (tmp[0] == MPI_SIGNAL_GET_CMD) {
      MPI_Send(&currentCommand, 1, MPI_INT, stat.MPI_SOURCE, MPI_TAG_TO_MASTER, _globalComm);
      currentCommand++;
    } else if (tmp[0] == MPI_SIGNAL_SEND_STATS) {
      Stats s;
      s= *((Stats*)&tmp[1]);
      _stats.push_back(s);
    } else if (tmp[0] == MPI_SIGNAL_GET_SORTED_CMD) {
      if (currentSortedCommand < (int)dimensions.size()) {
        MPI_Send(&dimensions[currentSortedCommand].command,
            3, MPI_INT, stat.MPI_SOURCE, MPI_TAG_TO_MASTER, _globalComm);
        currentSortedCommand++;
      } else {
        int buff[3];
        buff[0] = buff[1] = buff[2] = NO_MORE_CMDS;
        MPI_Send(buff,
            3, MPI_INT, stat.MPI_SOURCE, MPI_TAG_TO_MASTER, _globalComm);
      }
    } else {
      cerr << "Error in master_thread: unexpected signal " << tmp << endl; 
    }
  }
}

string get_random_hex()
{
  static const char *buff = "0123456789abcdef";
  char res[8];
  res[0] = '#';
  res[7] = 0;
  for (int i = 0; i < 6; ++i) {
    res[i+1] = buff[rand() % 16];
  }
  return string(res);
}

void Server::saveStats(const std::string &outputFile)
{
  if (!outputFile.size()) {
    cerr << "Server::saveStats: empty input file, skipping" << endl;
    return;
  }
  if (!_stats.size()) {
    cerr << "Server::saveStats: empty stats vector, skipping" << endl;
    return;  
  }
  std::cout << "saving svg in file " << outputFile << endl;
  ofstream os(outputFile, std::ofstream::out);
  if (!os) {
    cerr << "Server::saveStats: cannot open " << outputFile << ", skipping" << endl;
    return;  
  }
  // SVG header
  os << "<svg  xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">" << endl;

  int totalWidth = getSize(_globalComm);
  int totalHeight = 0;

  for (const auto &s: _stats) {
    totalHeight = max(totalHeight, s.startingTime + s.duration);
  }

  double ratioWidth = 1000.0 / totalWidth;
  double ratioHeight = 1000.0 / totalHeight;

  // for each registered statistic
  for (const auto &s: _stats) {

    os << "  <svg x=\"" << ratioWidth * s.startingRank 
       << "\" y=\"" << ratioHeight * s.startingTime << "\" "
       << "width=\"" << ratioWidth * s.ranks << "\" " 
       << "height=\""  << ratioHeight * s.duration << "\" >" << endl;
     
    string color = get_random_hex(); //hex(random.randrange(0, 16777216))[2:]
    os << "    <rect x=\"0%\" y=\"0%\" height=\"100%\" width=\"100%\" style=\"fill: "
       << color  <<  "\"/>" << endl;
    
    // uncomment to add some text
    //os << "   <text x=\"50%\" y=\"50%\" alignment-baseline=\"middle\" text-anchor=\"middle\">" 
    //   << text << "</text>" << endl;

    os << "  </svg>" << endl;
  }

  // SVG footer
  os << "</svg>" << endl;
}


}
