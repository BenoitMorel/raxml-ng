#ifndef _MULTIRAXML_CLIENT_HPP
#define _MULTIRAXML_CLIENT_HPP

#include <mpi.h>
#include "common.hpp"

namespace multiraxml {

void client_thread(const std::string &input_file, MPI_Comm globalComm, MPI_Comm localComm, Timer &begin); 


} // namespace


#endif

