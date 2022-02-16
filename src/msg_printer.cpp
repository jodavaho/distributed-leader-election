#include "msg_printer.hpp"

#include <sstream>

std::string to_string(const MsgType &type){
	switch (type){
		case MsgType::NOOP:{return "NOOP";}
		case MsgType::SRCH:{return "SRCH";}
		case MsgType::SRCH_RET:{return "SRCH_RET";}
		case MsgType::IN_PART:{return "IN_PART";}
		case MsgType::ACK_PART:{return "ACK_PART";}
		case MsgType::NACK_PART:{return "NACK_PART";}
		case MsgType::JOIN_US:{return "JOIN_US";}
		default: {return "??";};
	}
}

std::ostream& operator << ( std::ostream& outs, const MsgType & type )
{
  return outs << to_string(type);
}

std::ostream& operator << ( std::ostream& outs, const MsgData & d)
{
  MsgData local = d;
  size_t * v=reinterpret_cast<size_t*>(&local);
  outs << "(";
  outs << v[0]<<" ";
  outs << v[1]<<" ";
  outs << v[2]<<" ";
  outs << v[3];
  outs<<")";
  return outs;
}

std::ostream& operator << ( std::ostream& outs, const Msg & m)
{
  outs << "("<<m.from<<"-->"<< m.to<<") "<< m.type<<" {";
  outs << m.type<<" ";
  outs << m.data;
  outs<<" }";
  return outs;
}

