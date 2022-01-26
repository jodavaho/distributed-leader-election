#ifndef ghs_printer_hpp
#define ghs_printer_hpp

#include "ghs.hpp"
#include <string>
#include <iostream>

std::string dump_edges(const GhsState&) ;

std::ostream& operator << ( std::ostream& outs, const GhsState & s);

#endif
