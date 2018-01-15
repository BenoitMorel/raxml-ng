

#include "MultiRaxml.hpp"
#include "Raxml.hpp"

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

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
    while (is) {
      string str;
      is >> str;
      _args.push_back(str);
      LOG_INFO << str << endl;
    }
  }

  void run(MPI_Comm com) const {
    std::vector<char *> argv(_args.size() + 2, 0);
    argv[0] = "raxml";
    for (unsigned int i = 0; i < _args.size(); ++i) {
      argv [i + 1] = (char *)_args[i].c_str(); //todobenoit const hack here
    }
    raxml(argv.size() - 1, &argv[0]);
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
    commands.push_back(line);
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
  vector<RaxmlCommand> commands(commands_str.size());
  for (unsigned int i = 0; i < commands_str.size(); ++i) {
    commands[i] = RaxmlCommand(commands_str[i]);
  }
    for (const auto &command: commands) {
      command.run(MPI_COMM_WORLD);
    }
  return 0; 
}











