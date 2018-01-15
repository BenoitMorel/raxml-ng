

#include "MultiRaxml.hpp"
#include "Raxml.hpp"

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

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



int multi_raxml(int argc, char** argv)
{
  if (argc != 2) {
    LOG_ERROR << "Invalid syntax: multi-raxml requires one argument" << endl;
    return 0;
  }
  
  LOG_INFO << "Running multi_raxml with " << 
    ParallelContext::num_ranks() << "ranks" << std::endl;

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
  return 0; 
}











