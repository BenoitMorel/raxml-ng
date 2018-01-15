

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
    LOG_INFO << "Threads : " << _threadsNumber << endl;
    _args.push_back("raxml");
    while (is) {
      string str;
      is >> str;
      _args.push_back(str);
      LOG_INFO << str << endl;
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
    int tmp;
    MPI_Recv(&tmp, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
    if (tmp == -1) {
      return 0;
    }
    MPI_Send(&currentCommand, 1, MPI_INT, stat.MPI_SOURCE, 0, MPI_COMM_WORLD);
    currentCommand++;
    std::this_thread::sleep_for (std::chrono::milliseconds(20));
  }
  return 0;
}


int requestCurrentCommand() {
  if (ParallelContext::proc_id()) {
    int res = -1;
    int tmp = 0;
    MPI_Status stat;
    MPI_Sendrecv(&tmp, 1, MPI_INT, 0, 0, &res, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
    return res;
  } else {
    return currentCommand++;
  }
}

void *slaves_thread(void *) {
  for (unsigned int i = 0; i < 5; ++i) {
    std::this_thread::sleep_for (std::chrono::seconds(1));
    std::cout << "p" << ParallelContext::proc_id() << " c" << requestCurrentCommand() << std::endl;
  }
  /*
  string input_file = argv[1]; 
  vector<string> commands_str;
  read_commands_file(input_file, commands_str);
  vector<RaxmlCommand *> commands;
  for (const auto &str: commands_str) {
    commands.push_back(new RaxmlCommand(str));
  }
  std::sort(commands.rbegin(), commands.rend(), RaxmlCommand::comparePtr);
  
  for (auto command: commands) {
    command->run(MPI_COMM_WORLD);
  }
  
  for (auto command: commands) {
    delete command;
  }
  */
  return 0;
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
  slaves_thread(0);
  
  
  LOG_INFO << "todobenoit: remove this horrible termination hack" << std::endl;
  std::this_thread::sleep_for (std::chrono::milliseconds(1000));
  if (1 == ParallelContext::proc_id()) {
    int stop = -1;
    MPI_Send(&stop, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); 
  }
  std::this_thread::sleep_for (std::chrono::milliseconds(1000));
  return 0; 
}











