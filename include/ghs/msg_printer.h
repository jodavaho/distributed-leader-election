/**
 *
 * @file msg_printer.h
 * @brief provides the implementation of std::ostream operations for lg::ghs objects
 *
 */
#ifndef msg_printer_hpp
#define msg_printer_hpp
#include "msg.h"
#include <string>
#include <iostream>

std::string to_string(const le::ghs::Msg::Type &type);

std::ostream& operator << ( std::ostream& outs, const le::ghs::Msg::Type & type );

std::ostream& operator << ( std::ostream& outs, const le::ghs::Msg::Data & d);

std::ostream& operator << ( std::ostream& outs, const le::ghs::Msg & m);


#endif
