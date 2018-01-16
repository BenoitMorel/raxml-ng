/*
    Copyright (C) 2017 Alexey Kozlov, Alexandros Stamatakis, Diego Darriba, Tomas Flouri

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Contact: Alexey Kozlov <Alexey.Kozlov@h-its.org>,
    Heidelberg Institute for Theoretical Studies,
    Schloss-Wolfsbrunnenweg 35, D-69118 Heidelberg, Germany
*/

#include "Raxml.hpp"
#include "MultiRaxml.hpp"
#include "ParallelContext.hpp"
#include "log.hpp"
#include <iostream>
/**
 *  Normal Raxml run
 */
/*
int main(int argc, char** argv)
{
  ParallelContext::init_mpi(argc, argv);
  
  logger().add_log_stream(&cout);

  int res = raxml(argc, argv);

  clean_exit(res);

  return res;
}
*/

/**
 *  Multi-Raxml run
 */
int main(int argc, char** argv)
{
  ParallelContext::init_mpi(argc, argv);
 
  logger().add_log_stream(&std::cout);
  
  int res = multi_raxml(argc, argv);
  ParallelContext::set_comm(MPI_COMM_WORLD);
  clean_exit(res);

  return res;
}

