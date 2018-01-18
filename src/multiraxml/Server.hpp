#ifndef _MULTIRAXML_SERVER_HPP
#define _MULTIRAXML_SERVER_HPP

#include <mpi.h>

namespace multiraxml {

void server_thread(MPI_Comm globalComm);

} //namespace

#endif
