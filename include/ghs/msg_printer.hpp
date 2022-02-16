#ifndef msg_printer_hpp
#define msg_printer_hpp
#include "msg.hpp"
#include <string>
#include <iostream>

std::string to_string(const MsgType &type);

std::ostream& operator << ( std::ostream& outs, const MsgType & type );

std::ostream& operator << ( std::ostream& outs, const MsgData & d);

std::ostream& operator << ( std::ostream& outs, const Msg & m);


#endif
