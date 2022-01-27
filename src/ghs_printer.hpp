#ifndef ghs_printer_hpp
#define ghs_printer_hpp

#include "ghs.hpp"
#include <string>
#include <iostream>

template <std::size_t N, std::size_t A>
std::string dump_edges(const GhsState<N,A>&) ;

template <std::size_t N, std::size_t A>
std::ostream& operator << ( std::ostream&, const GhsState<N,A>&);

#include "ghs_printer_impl.hpp"

#endif
