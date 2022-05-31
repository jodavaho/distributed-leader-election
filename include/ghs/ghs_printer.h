/**
 * @file ghs_printer.h
 * @brief some conveneience implementations to print le::ghs::GhsState objects with std::ostream
 */
#ifndef ghs_printer_hpp
#define ghs_printer_hpp

#include "ghs/ghs.h"
#include <string>
#include <iostream>

/**
 * @brief Will dump edges in a readable format
 */
template <std::size_t N, std::size_t A>
std::string dump_edges(const le::ghs::GhsState<N,A>&) ;

/**
 * @brief Will dump the whole object in a readable format
 */
template <std::size_t N, std::size_t A>
std::ostream& operator << ( std::ostream&, const le::ghs::GhsState<N,A>&);

#include "ghs/ghs_printer_impl.hpp"

#endif
