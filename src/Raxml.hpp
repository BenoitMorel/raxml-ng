#ifndef RAXML_HPP_
#define RAXML_HPP_


int raxml(int argc, char** argv);
void clean_exit(int retval);
int get_dimensions(int argc, char** argv, int &sites, int &tips);


#endif
