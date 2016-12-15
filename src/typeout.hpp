/*
 * typeout.hpp
 *
 *  Created on: 5 Aug 2016
 *      Author: acolomitchi
 */

#ifndef TYPEOUT_HPP_
#define TYPEOUT_HPP_

// WARNING! GCC specific.
// TODO: replace it with a MP approach in printing type names!!!

#include <cstdlib>
#include <iostream>
#include <cxxabi.h>

template <typename aux> void outname(const char* givenType) {
  int status=-10;
  char* demangled=abi::__cxa_demangle(typeid(aux).name(), NULL, NULL, &status);
  std::cout << givenType << " m:" << typeid(aux).name() << " -> dm:" << demangled << std::endl;
  if(NULL!=demangled) {
    free(demangled);
  }
}



#endif /* TYPEOUT_HPP_ */
